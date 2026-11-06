package com.juanmote.outfoxies;

import android.content.Intent;

import org.libsdl.app.SDLActivity;

public class GameActivity extends SDLActivity {
    /** Mensaje del codigo nativo (main.c): "Cambiar ROM" en el menu. */
    private static final int MSG_CHANGE_ROM = 0x8001;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    @Override
    protected boolean onUnhandledMessage(int command, Object param) {
        if (command == MSG_CHANGE_ROM) {
            // El selector va en otro proceso (:picker), asi que este puede cerrarse del todo
            Intent i = new Intent(this, RomPickerActivity.class);
            i.putExtra(RomPickerActivity.EXTRA_CHANGE, true);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
            startActivity(i);
            finish();
            android.os.Process.killProcess(android.os.Process.myPid());
            return true;
        }
        return super.onUnhandledMessage(command, param);
    }
}
