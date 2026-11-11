package com.juanmote.outfoxies;

import android.content.Context;
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

    @Override
    protected void onDestroy() {
        try {
            if (netLock != null && netLock.isHeld()) netLock.release();
        } catch (Exception e) { }
        super.onDestroy();
    }
}
