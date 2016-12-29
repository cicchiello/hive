package com.jfc.apps.hive;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.util.Pair;
import android.widget.TextView;

public class HiveEnv {

	public static final boolean DEBUG = false;
	public static final boolean RELEASE_TEST = false;
	
    public static final String DbHost = "http://192.168.1.85";
    public static final int DbPort = 5984;
    public static final String Db = "hive-sensor-log";

	public static final int ModifiableBackgroundColor = 0xffC9EAEA;
	public static final int ModifiedFieldSplashColor = 0xff00FF21;
	
	private void highlightModifiedField(final Activity activity, final TextView t) {
		Runnable flair = new Runnable() {
			@Override
			public void run() {
				final int numSteps = 10;
				final int blastColor = HiveEnv.ModifiedFieldSplashColor;
				final int finalColor = HiveEnv.ModifiableBackgroundColor;
				int sr = ((blastColor >> 16) & 0xff), sg = ((blastColor >> 8) & 0xff), sb = ((blastColor >> 0) & 0xff);
				int fr = (finalColor&0x00ff0000 >> 16), fg = (finalColor&0x0000ff00 >> 8), fb = (finalColor&0x000000ff);
				float dr = (fr-sr)/(numSteps*1f), dg = (fg-sg)/(numSteps*1f), db = (fb-sb)/(numSteps*1f);
				for (int i = 0; i < numSteps; i++) {
					int nr = sr+((int) (i*dr+0.5));
					int ng = sg+((int) (i*dg+0.5));
					int nb = sb+((int) (i*db+0.5));
					final int c = 0xff000000 + (nr << 16) + (ng << 8) + nb;
					activity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							t.setBackgroundColor(c);
						}
					});
					try {
						Thread.sleep(200);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
				activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						t.setBackgroundColor(finalColor);
					}
				});
			}
		};
		Thread flairThread = new Thread(flair);
		flairThread.start();
	}

	static public String getHiveAddress(Context ctxt, String hiveName) {
    	List<Pair<String,String>> existingPairs = new ArrayList<Pair<String,String>>();
    	if (BluetoothAdapter.getDefaultAdapter() == null) {
    		// simulate one of my devices
    		// F0-17-66-FC-5E-A1
    		if (hiveName.equals("Joe's Hive")) 
    			return "F0-17-66-FC-5E-A1";
    		else 
    			return null;
    	} else {
	    	int sz = NumHivesProperty.getNumHivesProperty(ctxt);
	    	for (int i = 0; i < sz; i++) {
	    		if (PairedHiveProperty.getPairedHiveName(ctxt, i).equals(hiveName)) 
	    			return PairedHiveProperty.getPairedHiveId(ctxt, i);
	    	}
	    	return null;
    	}
    	
	}
	
}
