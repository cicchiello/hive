package com.jfc.srvc.ble2cld;

import com.adobe.xmp.impl.Base64;

import android.content.Context;
import android.util.Log;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.MotorProperty;
import com.jfc.apps.hive.SensorSampleRateProperty;
import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.UptimeProperty;

/**
 * Created by Joe on 12/12/2016.
 */

public class CmdProcess {
	private static final String TAG = MotorProperty.class.getName();

    public static boolean process(Context ctxt, String msg, String results[], CmdOnCompletion onCompletion) {
    	String tokens[] = msg.split("[|]");
    	if (tokens[0].equals("cmd")) {
    		String deviceName = tokens[1];
    		if (tokens[2].equals("POST")) {
    			String cloudantUser = DbCredentialsProperty.getCloudantUser(ctxt);
    			String dbName = DbCredentialsProperty.getDbName(ctxt);
    			String dbUrl = DbCredentialsProperty.CouchDbUrl(cloudantUser, dbName);
    			
    			String dbKey = DbCredentialsProperty.getDbKey(ctxt);
    			String dbPswd = DbCredentialsProperty.getDbPswd(ctxt);
    			String authToken = Base64.encode(dbKey+":"+dbPswd);
    			
                new CouchPostBackground(dbUrl, authToken, deviceName, tokens[4], onCompletion).execute();
    		} else if (tokens[2].equals("GETTIME")) {
    			String resetCause = tokens.length > 2 ? tokens[3] : "unknown";
            	long ms = System.currentTimeMillis();
            	long s = (long) ((ms+500l)/1000l);
            	UptimeProperty.setUptimeProperty(ctxt, s);
                onCompletion.complete("rply|"+deviceName+"|GETTIME|"+(s));
    		} else if (tokens[2].equals("GETSAMPLERATE")) {
    			if (ActiveHiveProperty.isActiveHivePropertyDefined(ctxt)) {
        			String activeHive = ActiveHiveProperty.getActiveHiveProperty(ctxt);
	    			String hiveId = HiveEnv.getHiveAddress(ctxt, activeHive);
	    			String rateStr = Integer.toString(SensorSampleRateProperty.getRate(ctxt));
    				String sensor = "sample-rate";
    				String rply = "action|"+sensor+"|"+rateStr;
    				onCompletion.complete(rply);
    			} else {
    				Log.e(TAG, "couldn't determine hiveid; can't send sample rate");
    			}
    		} else {
        		Log.e(TAG, "Unrecognized command: "+msg);
    		}
    	} else {
    		Log.e(TAG, "Unrecognized command: "+msg);
    	}
        return false;
    }
}
