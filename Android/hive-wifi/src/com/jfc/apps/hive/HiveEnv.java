package com.jfc.apps.hive;

import java.util.Calendar;
import java.util.Locale;

import org.json.JSONObject;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.NumHivesProperty;
import com.jfc.misc.prop.PairedHiveProperty;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.util.misc.SplashyText;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.text.format.DateFormat;
import android.widget.TextView;

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
	
	public static void couchGetConfig(Context ctxt, final CouchGetBackground.OnCompletion onCompletion) {
		if (ActiveHiveProperty.isActiveHivePropertyDefined(ctxt)) {
	    	CouchGetBackground.OnCompletion couchOnCompletion = onCompletion;
			String dbUrl = DbCredentialsProperty.getCouchConfigDbUrl(ctxt);
			String authToken = null;
			String hiveId = PairedHiveProperty.getPairedHiveId(ctxt, ActiveHiveProperty.getActiveHiveIndex(ctxt));
	    	final CouchGetBackground getter = new CouchGetBackground(dbUrl+"/"+hiveId, authToken, couchOnCompletion);
	    	getter.execute();
		}
	}

	
	public static void setValueImplementation(Activity activity, 
											  int valueResid, int timestampResid, 
											  String value, boolean isTimestampDefined, long timestamp, 
											  boolean addSplash) 
	{
		TextView valueTv = valueResid != 0 ? (TextView) activity.findViewById(valueResid) : null;
		TextView timestampTv = (TextView) activity.findViewById(timestampResid);
		boolean splashValue = valueTv != null ? !valueTv.getText().equals(value) : false;
		if (valueTv != null) valueTv.setText(value);
		boolean splashTimestamp = false;
		if (isTimestampDefined) {
			Calendar cal = Calendar.getInstance(Locale.ENGLISH);
			cal.setTimeInMillis(timestamp);
			String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
			splashTimestamp = !timestampTv.getText().equals(timestampStr);
			timestampTv.setText(timestampStr);
		} else {
			splashTimestamp = !timestampTv.getText().equals("<unknown>");
			timestampTv.setText("<unknown>");
		}
		if (addSplash) {
			if (splashValue) 
				SplashyText.highlightModifiedField(activity, valueTv);
			if (splashTimestamp) 
				SplashyText.highlightModifiedField(activity, timestampTv);
		}
	}

	public static void setValue(Activity activity, int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(activity, valueResid, timestampResid, value, isTimestampDefined, timestamp, false);
	}

	public static void setValueWithSplash(Activity activity, int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(activity, valueResid, timestampResid, value, isTimestampDefined, timestamp, true);
	}

}
