package com.jfc.srvc.ble2cld;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class SimpleWakefulReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context ctxt, Intent intent) {
		Intent ble2cldIntent= new Intent(ctxt, BluetoothPipeSrvc.class);
		ble2cldIntent.putExtra("cmd", "setup");
		ctxt.startService(ble2cldIntent);
    }
    
}
