package com.jfc.apps.hive;

import android.os.AsyncTask;
import android.util.Log;

import org.apache.http.client.ClientProtocolException;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;

/**
 * Created by Joe on 12/12/2016.
 */

public class PollSensorBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = PollSensorBackground.class.getName();

    private String dbHost, db, key, pswd, query;
    private int dbPort;
    private String docId, rev;
    private ResultCallback onCompletion;

    public interface ResultCallback {
    	public void report(String sensorType, String timestampStr, String valueStr);
    	public void error(String msg);
    };
    
    public PollSensorBackground(String _dbHost, int _dbPort, String _db, String _key, String _pswd, String _query, ResultCallback _onCompletion) {
        dbHost = _dbHost;
        dbPort = _dbPort;
        db = _db;
        key = _key;
        pswd = _pswd;
        query = _query;
        onCompletion = _onCompletion;
    }

    public String getDocId() {return docId;}
    public String getRev() {return rev;}
    
    interface ProcessResult {
    	public void onSuccess(JSONObject obj);
    	public void onError(String msg);
    };
    
    private static void couchGet(String dbHost, int dbPort, String dbKey, String dbPswd, String db, String query, String authToken, ProcessResult proc) {
        String urlStub = "http://" + dbKey + ":" + dbPswd + "@" + dbHost+":"+dbPort+"/"+db+"/"+query;

		HttpURLConnection conn = null;
		BufferedReader rd = null;
		InputStreamReader isr = null;
		InputStream is = null;
		try {
            URL url = new URL(urlStub);
			conn = (HttpURLConnection) url.openConnection();
			conn.setRequestMethod("GET");
			if (authToken != null)
				conn.setRequestProperty("Authorization", "Basic "+authToken);
			
			rd = new BufferedReader(isr = new InputStreamReader(is = conn.getInputStream()));
            StringBuilder builder = new StringBuilder();
            for (String line = null; (line = rd.readLine()) != null;)
                builder.append(line).append("\n");

            //System.err.println("Response: "+builder);

            JSONObject r = new JSONObject(new JSONTokener(builder.toString()));
            proc.onSuccess(r);
        } catch (ClientProtocolException e) {
            proc.onError("error: "+"ClientProtocolException: "+e);
            System.err.println("ClientProtocolException: "+e);
        } catch (IOException e) {
        	proc.onError("error: "+"IOException: "+e);
            System.err.println("IOException: "+e);
        } catch (JSONException e) {
        	proc.onError("error: "+"JSONException: "+e);
            System.err.println("JSONException: " + e);
        } catch (Exception e) {
        	proc.onError("error: "+"Exception: "+e);
            System.err.println("Exception: " + e);
        } finally {
            try {
                if (conn != null) {
                	conn.disconnect();
                }
                if (rd != null) rd.close();
                if (is != null) is.close();
            } catch (IOException ioe) {
                // ignore
            }
            rd = null;
            is = null;
            conn = null;
        }
    }
    
    @Override
    protected Boolean doInBackground(Void... params) {
        final String authToken = null;
        ProcessResult latestProc = new ProcessResult() {

			@Override
			public void onSuccess(JSONObject obj) {
	            try {
	            	JSONArray arr = obj.getJSONArray("rows");
	            	if (!arr.isNull(0)) {
	            		JSONArray v = arr.getJSONObject(0).getJSONArray("value");
	            		String sensorStr = v.getString(1);
	            		String timestampStr = v.getString(2);
	            		String valueStr = v.getString(3);
	            		if (onCompletion != null)
	            			onCompletion.report(sensorStr, timestampStr, valueStr);
	            	} else {
	            		Log.e(TAG, "Unexpected response from couchDb: "+obj.toString());
	            		Log.e(TAG, "Response resulted from query: "+query);
	            	}
				} catch (JSONException e) {
					onError("error Parsing JSON result for query: "+query);
				}
			}

			@Override
			public void onError(String msg) {
				Log.e(TAG, msg);
			}
		};
        couchGet(dbHost, dbPort, key, pswd, db, query, authToken, latestProc);
        return true;
    }

}

