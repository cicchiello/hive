package com.jfc.apps.hive;

import org.json.JSONObject;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.NumHivesProperty;
import com.jfc.misc.prop.PairedHiveProperty;
import com.jfc.srvc.cloud.CouchGetBackground;

import android.bluetooth.BluetoothAdapter;
import android.content.Context;

public class HiveEnv {

	public static final boolean DEBUG = false;
	public static final boolean RELEASE_TEST = false;
	
	public static final int ModifiableFieldBackgroundColor = 0xffC9EAEA;
	public static final int ModifiableFieldSplashColor = 0xff00FF21;
	public static final int ModifiableFieldErrorColor = 0xffFF0000;
	
	static public String getHiveAddress(Context ctxt, String hiveName) {
    	int sz = NumHivesProperty.getNumHivesProperty(ctxt);
    	for (int i = 0; i < sz; i++) {
    		if (PairedHiveProperty.getPairedHiveName(ctxt, i).equals(hiveName)) 
    			return PairedHiveProperty.getPairedHiveId(ctxt, i);
    	}
    	return null;
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
	
	public interface CouchGetConfig_onCompletion {
		public void failed(String msg);
		public void complete(JSONObject doc);
	}

	public static void couchGetConfig(Context ctxt, final CouchGetConfig_onCompletion onCompletion) {
		if (ActiveHiveProperty.isActiveHivePropertyDefined(ctxt)) {
	    	CouchGetBackground.OnCompletion couchOnCompletion = new CouchGetBackground.OnCompletion() {
				@Override
				public void failed(final String msg) {
					onCompletion.failed(msg);
				}
				
				@Override
				public void complete(JSONObject resultDoc) {
					onCompletion.complete(resultDoc);
				}
			};
			String dbUrl = DbCredentialsProperty.getCouchConfigDbUrl(ctxt);
			String authToken = null;
			String hiveId = PairedHiveProperty.getPairedHiveId(ctxt, ActiveHiveProperty.getActiveHiveIndex(ctxt));
	    	final CouchGetBackground getter = new CouchGetBackground(dbUrl+"/"+hiveId, authToken, couchOnCompletion);
	    	getter.execute();
		}
	}
	
}
