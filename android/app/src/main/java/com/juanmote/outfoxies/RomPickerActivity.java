package com.juanmote.outfoxies;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** Primera pantalla: comprueba que la ROM del usuario esta instalada y arranca el juego. */
public class RomPickerActivity extends Activity {
    private static final int PICK_ROM = 1;
    private static final String[] REQUIRED = {
        "ou2_mpru.11d", "ou2_mprl.11c", "ou1dat0.20a", "ou1dat1.20b", "ou1-scr0.1d", "ou1shas.12s", "ou1shar.18s"
    };
    private TextView status;
    private Button pick;

    private static String t(String en, String es) {
        return "es".equals(Locale.getDefault().getLanguage()) ? es : en;
    }

    private File romFile() { return new File(getFilesDir(), "rom.zip"); }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (romFile().exists()) {
            startGame();
            return;
        }
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setBackgroundColor(Color.rgb(12, 12, 16));
        root.setPadding(48, 48, 48, 48);

        TextView title = new TextView(this);
        title.setText("THE OUTFOXIES");
        title.setTextColor(Color.rgb(240, 150, 40));
        title.setTextSize(28);
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        status = new TextView(this);
        status.setText(t("To play you need your own copy of the arcade romset: outfxies.zip (World, OU2).\n"
                + "You only have to choose it once: it will be copied inside the app.",
                "Para jugar necesitas tu copia del romset del arcade: outfxies.zip (World, OU2).\n"
                + "Solo hay que elegirlo una vez: se copiara dentro de la app."));
        status.setTextColor(Color.WHITE);
        status.setTextSize(16);
        status.setGravity(Gravity.CENTER);
        status.setPadding(0, 32, 0, 32);
        root.addView(status);

        pick = new Button(this);
        pick.setText(t("Choose ROM", "Elegir ROM"));
        pick.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openPicker(); }
        });
        root.addView(pick);
        setContentView(root);
        pick.requestFocus();
    }

    private void openPicker() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        startActivityForResult(i, PICK_ROM);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_ROM || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        final Uri uri = data.getData();
        pick.setEnabled(false);
        status.setText(t("Checking the file...", "Comprobando el archivo..."));
        new Thread(new Runnable() {
            @Override public void run() { install(uri); }
        }).start();
    }

    private void install(Uri uri) {
        File tmp = new File(getFilesDir(), "rom.tmp");
        try {
            InputStream in = getContentResolver().openInputStream(uri);
            FileOutputStream out = new FileOutputStream(tmp);
            byte[] buf = new byte[1 << 16];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
            out.close();
            in.close();
            String missing = firstMissing(tmp);
            if (missing != null) {
                tmp.delete();
                show(t("That is not the right romset (missing " + missing + "). Use outfxies.zip, World version (OU2).",
                       "Ese no es el romset correcto (falta " + missing + "). Usa outfxies.zip, version World (OU2)."));
                return;
            }
            if (!tmp.renameTo(romFile())) throw new Exception("rename");
            runOnUiThread(new Runnable() {
                @Override public void run() { startGame(); }
            });
        } catch (Exception e) {
            tmp.delete();
            show(t("Could not read the file: ", "No se pudo leer el archivo: ") + e.getMessage());
        }
    }

    /** Devuelve el primer archivo necesario que falta, o null si estan todos. */
    private static String firstMissing(File zip) throws Exception {
        Set<String> names = new HashSet<String>();
        ZipInputStream z = new ZipInputStream(new java.io.FileInputStream(zip));
        ZipEntry e;
        while ((e = z.getNextEntry()) != null) names.add(e.getName());
        z.close();
        for (String r : REQUIRED) if (!names.contains(r)) return r;
        return null;
    }

    private void show(final String msg) {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                status.setText(msg);
                pick.setEnabled(true);
                pick.requestFocus();
            }
        });
    }

    private void startGame() {
        startActivity(new Intent(this, GameActivity.class));
        finish();
    }
}
