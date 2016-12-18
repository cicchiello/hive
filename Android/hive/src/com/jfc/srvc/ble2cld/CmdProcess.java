package com.jfc.srvc.ble2cld;

/**
 * Created by Joe on 12/12/2016.
 */

public class CmdProcess {
    private static final String DbHost = "http://admin:admin@192.168.1.85";
    private static final int DbPort = 5984;

    public static boolean process(String msg, String results[], CmdOnCompletion onCompletion) {
    	String cmd = msg.substring("cmd|".length()).trim();
    	if (cmd.startsWith("POST")) {
            String tokens[] = cmd.split("[|]");
            new CouchPostBackground(DbHost, DbPort, tokens[1], tokens[2], onCompletion).execute();
        } else if (cmd.startsWith("GETTIME")) {
        	long ms = System.currentTimeMillis();
        	long s = (long) ((ms+500l)/1000l);
            onCompletion.complete("rply|GETTIME|"+((long)((System.currentTimeMillis()+500)/1000)));
        }
        return false;
    }
}
