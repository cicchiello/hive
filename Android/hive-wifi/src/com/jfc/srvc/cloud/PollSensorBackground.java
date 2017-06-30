package com.jfc.srvc.cloud;

import android.app.Activity;
import android.os.AsyncTask;
import android.util.Log;

import org.acra.ACRA;
import org.apache.http.client.ClientProtocolException;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import com.jfc.apps.hive.R;
import com.jfc.util.misc.DbAlertHandler;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.ConnectException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.UnknownHostException;

/**
 * Created by Joe on 12/12/2016.
 */

public class PollSensorBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = PollSensorBackground.class.getName();

    private String dbUrl, query, sensor;
    private String docId, rev;
    private ResultCallback onCompletion;

    public interface ResultCallback {
    	public void report(String objId, String sensorType, String timestampStr, String valueStr);
    	public void error(String msg);
    	public void dbAccessibleError(String msg);
    };
    
    public PollSensorBackground(String _dbUrl, String _query, String _sensor, ResultCallback _onCompletion) {
    	dbUrl = _dbUrl;
        query = _query;
        sensor = _sensor;
        onCompletion = _onCompletion;
    }

    public String getDocId() {return docId;}
    public String getRev() {return rev;}
    
	public interface OnSaveValue {
		public void save(Activity activity, String objId, String value, long timestamp);
	};

	static public String createQuery(String hiveId, String sensor) {
		String rangeStartKeyClause = "[\"" + hiveId + "\",\""+sensor+"\",\"9999999999\"]";
		String rangeEndKeyClause = "[\"" + hiveId + "\",\""+sensor+"\",\"0000000000\"]";
		String query = "_design/SensorLog/_view/by-hive-sensor?endKey=" + rangeEndKeyClause + "&startkey=" + rangeStartKeyClause + "&descending=true&limit=1";
		return query;
	}
	
	static public ResultCallback getSensorOnCompletion(final Activity activity, 
													   final String sensorName, 
													   final int valueResid, 
													   final DbAlertHandler dbAlert, 
													   final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
			public void report(final String objId, final String sensorType, final String timestampStr, final String valueStr) {
				activity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if (sensorName.equals(sensorType)) {
							long timestampSeconds = Long.parseLong(timestampStr);
							long timestampMillis = timestampSeconds*1000;
							saver.save(activity, objId, valueStr, timestampSeconds);
						}
					}
				});
			}
		
			@Override
			public void error(final String msg) {
				String errMsg = "Attempt to get Sensor state failed with this message: "+msg;
				dbAlert.informDbInaccessible(activity, errMsg, valueResid);
			}
			
			@Override
			public void dbAccessibleError(final String msg) {
				dbAlert.informDbInaccessible(activity, activity.getString(R.string.db_inaccessible), valueResid);
				ACRA.getErrorReporter().handleSilentException(new Exception("dbInaccessible: "+msg));
			}
		};
		return onCompletion;
	}

	
    interface ProcessResult {
    	public void onSuccess(JSONObject obj);
    	public void onError(String msg);
    	public void couchInaccessibleError(String msg);
    };
    
    private static void couchGet(String dbUrl, String query, ProcessResult proc) {
    	String urlStub = dbUrl+"/"+query;

		HttpURLConnection conn = null;
		BufferedReader rd = null;
		InputStreamReader isr = null;
		InputStream is = null;
		try {
            URL url = new URL(urlStub);
			conn = (HttpURLConnection) url.openConnection();
			conn.setRequestMethod("GET");
			
			rd = new BufferedReader(isr = new InputStreamReader(is = conn.getInputStream()));
            StringBuilder builder = new StringBuilder();
            for (String line = null; (line = rd.readLine()) != null;)
                builder.append(line).append("\n");

            //System.err.println("Response: "+builder);

            JSONObject r = new JSONObject(new JSONTokener(builder.toString()));
            proc.onSuccess(r);
        } catch (ConnectException ce) {
        	proc.onError(ce.getLocalizedMessage());
        } catch (ClientProtocolException e) {
            proc.onError("error: "+"ClientProtocolException: "+e);
            System.err.println("ClientProtocolException: "+e);
			ACRA.getErrorReporter().handleException(e);
        } catch (UnknownHostException e) {
        	proc.couchInaccessibleError("error: "+"UnknownHostException: "+e);
            System.err.println("UnknownHostException: "+e);
        } catch (FileNotFoundException e) {
        	proc.couchInaccessibleError("error: "+"FileNotFoundException: "+e);
            System.err.println("UnknownHostException: "+e);
        } catch (IOException e) {
        	proc.onError("error: "+"IOException: "+e);
            System.err.println("IOException: "+e);
			ACRA.getErrorReporter().handleException(e);
        } catch (JSONException e) {
        	proc.onError("error: "+"JSONException: "+e);
            System.err.println("JSONException: " + e);
			ACRA.getErrorReporter().handleException(e);
        } catch (Exception e) {
        	proc.onError("error: "+"Exception: "+e);
            System.err.println("Exception: " + e);
			ACRA.getErrorReporter().handleException(e);
        } finally {
            try {
                if (conn != null) {
                	conn.disconnect();
                }
                if (rd != null) rd.close();
                if (is != null) is.close();
                if (isr != null) isr.close();
            } catch (IOException ioe) {
                // ignore
            }
            rd = null;
            isr = null;
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
	            		String objId = arr.getJSONObject(0).getString("id");
	            		String sensorStr = v.getString(1);
	            		String timestampStr = v.getString(2);
	            		String valueStr = v.getString(3);
	            		if (sensorStr.equals(sensor)) // else bogus query response since it couldn't be a closed query
	            			if (onCompletion != null)
	            				onCompletion.report(objId, sensorStr, timestampStr, valueStr);
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
				if (onCompletion != null)
					onCompletion.error(msg);
			}

			@Override
			public void couchInaccessibleError(String msg) {
				if (onCompletion != null)
					onCompletion.dbAccessibleError(msg);
			}
		};
        couchGet(dbUrl, query, latestProc);
        return true;
    }

}

