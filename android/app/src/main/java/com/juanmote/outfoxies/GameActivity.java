package com.juanmote.outfoxies;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
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
       galeria.zip o manual.zip, en otro hilo y pasando por un temporal: el visor lo abre cuando aparece. */
    private static final int MSG_PICK_PACK = 0x8007;
    private static final int PICK_PACK = 7;
    private int packKind = 1;

    @Override
    protected boolean onUnhandledMessage(int command, Object param) {
        if (command == MSG_PICK_PACK) {
            if (param instanceof Integer) packKind = (Integer) param;
            runOnUiThread(new Runnable() {
                @Override public void run() {
                    try {
                        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                        i.addCategory(Intent.CATEGORY_OPENABLE);
                        i.setType("*/*");
                        i.putExtra(Intent.EXTRA_MIME_TYPES, new String[] { "application/zip", "application/x-zip-compressed", "application/octet-stream" });
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
        if (requestCode != PICK_PACK || resultCode != Activity.RESULT_OK || data == null || data.getData() == null) return;
        final Uri uri = data.getData();
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

    @Override
    protected void onDestroy() {
        try {
            if (netLock != null && netLock.isHeld()) netLock.release();
        } catch (Exception e) { }
        super.onDestroy();
    }
}
