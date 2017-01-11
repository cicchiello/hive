package com.jfc.apps.hive;

import java.util.ArrayList;
import java.util.List;

import com.jfc.misc.prop.NumHivesProperty;
import com.jfc.misc.prop.PairedHiveProperty;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.util.Pair;
import android.widget.TextView;

public class HiveEnv {

	public static final boolean DEBUG = false;
	public static final boolean RELEASE_TEST = false;
	
	public static final boolean IsProd = true;
	
	public static final int ModifiableFieldBackgroundColor = 0xffC9EAEA;
	public static final int ModifiableFieldSplashColor = 0xff00FF21;
	public static final int ModifiableFieldErrorColor = 0xffFF0000;
	
	static public String getHiveAddress(Context ctxt, String hiveName) {
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
	
	static public String getHiveAddress(Context ctxt, int hiveIndex) {
    	if (BluetoothAdapter.getDefaultAdapter() == null) {
    		// simulate one of my devices
			return "F0-17-66-FC-5E-A1";
    	} else {
	    	int sz = NumHivesProperty.getNumHivesProperty(ctxt);
	    	if (hiveIndex < sz) 
	    		return PairedHiveProperty.getPairedHiveId(ctxt, hiveIndex);
	    	else
	    		return "error";
    	}
	}
	
}
