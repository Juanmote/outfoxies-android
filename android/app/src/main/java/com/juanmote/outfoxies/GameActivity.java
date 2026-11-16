package com.juanmote.outfoxies;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import android.net.wifi.WifiManager;
import android.os.Bundle;

import org.libsdl.app.SDLActivity;

public class GameActivity extends SDLActivity {
    private WifiManager.MulticastLock netLock;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    /* Juego en red: para encontrarse en la wifi las consolas se mandan avisos "a todos" (difusion UDP), y algunos
       aparatos los tiran si la app no pide este permiso de la wifi mientras esta abierta. */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            WifiManager wm = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wm != null) {
                netLock = wm.createMulticastLock("outfoxies-red");
                netLock.setReferenceCounted(false);
                netLock.acquire();
            }
        } catch (Exception e) {
            netLock = null;
        }
    }

    /* GALERIA y MANUAL del launcher (viewer.c): la app pide el .zip del pack con el mensaje 0x8007 (param 1 galeria,
       2 manual); se abre el selector de archivos y el .zip se copia a la carpeta de la app (la de las partidas) como
       galeria.zip o manual.zip, en otro hilo y pasando por un temporal: el visor lo abre cuando aparece.
       ROPA (skineditor.c): param 3 importa un pack de ropa (.txt, o un .zip con varios) a la carpeta "ropas"; param 4
       guarda donde diga el usuario el pack que ha dejado el editor en ropa_exportar.txt (nombre propuesto en
       ropa_exportar.nombre). */
    private static final int MSG_PICK_PACK = 0x8007;
    private static final int PICK_PACK = 7;
    private static final int SAVE_SKIN = 8;
    private int packKind = 1;

    @Override
    protected boolean onUnhandledMessage(int command, Object param) {
        if (command == MSG_PICK_PACK) {
            if (param instanceof Integer) packKind = (Integer) param;
            if (packKind == 4) { exportSkin(); return true; }
            runOnUiThread(new Runnable() {
                @Override public void run() {
                    try {
                        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                        i.addCategory(Intent.CATEGORY_OPENABLE);
                        i.setType("*/*");
                        if (packKind == 3) i.putExtra(Intent.EXTRA_MIME_TYPES, new String[] { "text/plain", "application/zip", "application/x-zip-compressed", "application/octet-stream" });
                        else i.putExtra(Intent.EXTRA_MIME_TYPES, new String[] { "application/zip", "application/x-zip-compressed", "application/octet-stream" });
                        startActivityForResult(i, PICK_PACK);
                    } catch (Exception e) {
                        try {
                            Intent i = new Intent(Intent.ACTION_GET_CONTENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);
                            i.setType("*/*");
                            startActivityForResult(i, PICK_PACK);
                        } catch (Exception e2) { }
                    }
                }
            });
            return true;
        }
        return super.onUnhandledMessage(command, param);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != Activity.RESULT_OK || data == null || data.getData() == null) return;
        if (requestCode == SAVE_SKIN) { saveSkin(data.getData()); return; }
        if (requestCode != PICK_PACK) return;
        final Uri uri = data.getData();
        if (packKind == 3) { importSkin(uri); return; }
        final String name = packKind == 2 ? "manual.zip" : "galeria.zip";
        File d = getExternalFilesDir(null);
        if (d == null) d = getFilesDir();
        final File dir = d;
        new Thread(new Runnable() {
            @Override public void run() {
                File tmp = new File(dir, name + ".tmp"), out = new File(dir, name);
                try {
                    InputStream in = getContentResolver().openInputStream(uri);
                    OutputStream os = new FileOutputStream(tmp);
                    byte[] buf = new byte[1 << 16];
                    int n;
                    while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
                    os.close(); in.close();
                    out.delete();
                    if (!tmp.renameTo(out)) tmp.delete();
                } catch (Exception e) { tmp.delete(); }
            }
        }).start();
    }

    /* ---------- packs de ropa ---------- */
    private File appDir() { File d = getExternalFilesDir(null); return d != null ? d : getFilesDir(); }
    private static String cleanName(String n) {
        if (n == null) n = "Ropa";
        n = n.replaceAll("[\\\\/:*?\"<>|]", "").trim();
        if (n.toLowerCase().endsWith(".txt")) n = n.substring(0, n.length() - 4).trim();
        if (n.length() > 40) n = n.substring(0, 40).trim();
        return n.isEmpty() ? "Ropa" : n;
    }
    private String displayName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null)) {
            if (c != null && c.moveToFirst()) return c.getString(0);
        } catch (Exception e) { }
        String p = uri.getLastPathSegment();
        return p != null ? p.substring(p.lastIndexOf('/') + 1) : "Ropa";
    }
    /* copia un pack a ropas/<nombre>.txt pasando por un temporal fuera de la carpeta (el juego no ve uno a medias) */
    private static void putSkin(File dir, String name, InputStream in) throws Exception {
        File rop = new File(dir, "ropas"); rop.mkdirs();
        File tmp = new File(dir, "ropa_importar.tmp"), out = new File(rop, cleanName(name) + ".txt");
        OutputStream os = new FileOutputStream(tmp);
        byte[] buf = new byte[1 << 14];
        int n, total = 0;
        while ((n = in.read(buf)) > 0 && total < (1 << 20)) { os.write(buf, 0, n); total += n; }
        os.close();
        out.delete();
        if (!tmp.renameTo(out)) tmp.delete();
    }
    private void importSkin(final Uri uri) {
        final File dir = appDir();
        final String name = displayName(uri);
        new Thread(new Runnable() {
            @Override public void run() {
                try {
                    BufferedInputStream in = new BufferedInputStream(getContentResolver().openInputStream(uri));
                    in.mark(4);
                    int a = in.read(), b = in.read();
                    in.reset();
                    if (a == 'P' && b == 'K') {   /* .zip: todos los .txt que lleve */
                        ZipInputStream z = new ZipInputStream(in);
                        ZipEntry e;
                        while ((e = z.getNextEntry()) != null) {
                            String n = e.getName(); n = n.substring(n.lastIndexOf('/') + 1);
                            if (!e.isDirectory() && n.toLowerCase().endsWith(".txt") && !n.startsWith(".")) putSkin(dir, n, z);
                        }
                        z.close();
                    } else { putSkin(dir, name, in); in.close(); }
                } catch (Exception e) { }
            }
        }).start();
    }
    private void exportSkin() {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                String title = "Ropa.txt";
                try (FileInputStream f = new FileInputStream(new File(appDir(), "ropa_exportar.nombre"))) {
                    byte[] b = new byte[256]; int n = f.read(b);
                    if (n > 0) title = new String(b, 0, n, "UTF-8").trim();
                } catch (Exception e) { }
                try {
                    Intent i = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                    i.addCategory(Intent.CATEGORY_OPENABLE);
                    i.setType("text/plain");
                    i.putExtra(Intent.EXTRA_TITLE, title);
                    startActivityForResult(i, SAVE_SKIN);
                } catch (Exception e) { }
            }
        });
    }
    private void saveSkin(final Uri uri) {
        final File src = new File(appDir(), "ropa_exportar.txt");
        new Thread(new Runnable() {
            @Override public void run() {
                try (InputStream in = new FileInputStream(src); OutputStream os = getContentResolver().openOutputStream(uri, "wt")) {
                    byte[] buf = new byte[1 << 14];
                    int n;
                    while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
                } catch (Exception e) { }
            }
        }).start();
    }

    @Override
    protected void onDestroy() {
        try {
            if (netLock != null && netLock.isHeld()) netLock.release();
        } catch (Exception e) { }
        super.onDestroy();
    }
}
