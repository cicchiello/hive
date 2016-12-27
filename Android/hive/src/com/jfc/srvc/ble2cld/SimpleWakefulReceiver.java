package com.jfc.srvc.ble2cld;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class SimpleWakefulReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context ctxt, Intent intent) {
		Intent mBle2cldIntent= new Intent(ctxt, BluetoothPipeSrvc.class);
		ctxt.startService(mBle2cldIntent);
    }
}