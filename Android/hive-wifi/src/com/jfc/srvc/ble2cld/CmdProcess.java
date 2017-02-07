package com.jfc.srvc.ble2cld;

import java.util.HashMap;
import java.util.Map;

import android.content.Context;
import android.util.Log;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.UptimeProperty;

/**
 * Created by Joe on 12/12/2016.
 */

public class CmdProcess {
	private static final String TAG = CmdProcess.class.getName();

	public interface Processor {
		public void process(Context ctxt, String tokens[], CmdOnCompletion onCompletion);
	};

	private static CmdProcess s_singleton = new CmdProcess();
	private Map<String,Processor> cmdRegistry = new HashMap<String,Processor>();
	
	public static CmdProcess singleton() {
		return s_singleton;
	}
	
	public void registerCmd(String name, Processor p) {
		cmdRegistry.put(name, p);
	}
	
    public boolean process(Context ctxt, String msg, String results[], CmdOnCompletion onCompletion) {
    	Log.i(TAG, "Received: "+msg);
    	String tokens[] = msg.split("[|]");
    	if (tokens[0].equals("cmd")) {
			if (cmdRegistry.containsKey(tokens[2])) {
				cmdRegistry.get(tokens[2]).process(ctxt, tokens, onCompletion);
			} else {
				Log.e(TAG, "Unrecognized command: "+msg);
			}
    	} else if (tokens[0].equals("noop")) {
    		// no-op
    		Log.i(TAG, "No-op received");
    	}
        return false;
    }

	private CmdProcess() {
		registerCmd("GETTIME", new Processor() {
			@Override
			public void process(Context ctxt, String[] tokens, CmdOnCompletion onCompletion) {
    			final int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(ctxt);
    			String resetCause = tokens.length > 2 ? tokens[3] : "unknown";
    			String versionId = tokens.length > 3 ? tokens[4] : "0.0";
            	long ms = System.currentTimeMillis();
            	long s = (long) ((ms+500l)/1000l);
            	UptimeProperty.setUptimeProperty(ctxt, hiveIndex, s);
            	UptimeProperty.setEmbeddedVersion(ctxt, hiveIndex, versionId);
                onCompletion.complete("rply|"+tokens[1]+"|GETTIME|"+(s));
			}
		});
	}
	
}
