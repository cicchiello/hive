package com.jfc.srvc.cloud;

import android.os.AsyncTask;
import android.util.Log;

import org.apache.http.client.HttpResponseException;
import org.apache.http.client.ResponseHandler;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.UnknownHostException;

/**
 * Created by Joe on 12/12/2016.
 */

public class PostActuatorBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = PostActuatorBackground.class.getName();

    private String dbUrl, authToken;
    private JSONObject doc;
    private String docId, rev;
    private ResultCallback onCompletion;

    public interface ResultCallback {
    	public void success(String id, String rev);
    	public void error(String msg);
    	public void serviceUnavailable(String msg);
    };
    
    public PostActuatorBackground(String _dbUrl, String _authToken, JSONObject _doc, ResultCallback _onCompletion) {
    	dbUrl = _dbUrl;
    	authToken = _authToken;
        doc = _doc;
        onCompletion = _onCompletion;
    }

    public String getDocId() {return docId;}
    public String getRev() {return rev;}
    
    interface ProcessResult {
    	public void onSuccess(JSONObject obj);
    	public void onError(String msg);
    	public void onServiceUnavailable(String msg);
    };
    
    private static void couchPost(String dbUrl, JSONObject doc, String authToken, ProcessResult proc) {
        String urlStub = dbUrl;

        DefaultHttpClient httpclient;
        HttpPost httpPost;
        try {
            httpclient = new DefaultHttpClient();
            httpPost = new HttpPost(urlStub);
            StringEntity se = new StringEntity(doc.toString());
            httpPost.setEntity(se);
            httpPost.setHeader("Accept", "application/json");
            httpPost.setHeader("Content-type", "application/json");
            if (authToken != null)
            	httpPost.addHeader("Authorization", "Basic "+authToken);
            
            //Handles what is returned from the page 
            ResponseHandler responseHandler = new BasicResponseHandler();
            
            try {
            	proc.onSuccess(new JSONObject(new JSONTokener((String) httpclient.execute(httpPost, responseHandler))));
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				proc.onError(e.getLocalizedMessage());
			}
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        } catch (HttpResponseException e) {
        	if (e.getStatusCode()==503) {
        		proc.onServiceUnavailable(e.getLocalizedMessage());
        	} else {
        		proc.onError(e.getLocalizedMessage());
        	}
        } catch (UnknownHostException e) {
        	proc.onError(e.getLocalizedMessage());
        	System.err.println("UnknownHostException: "+e);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
        }
    }
    
    @Override
    protected Boolean doInBackground(Void... params) {
        ProcessResult latestProc = new ProcessResult() {

			@Override
			public void onSuccess(JSONObject obj) {
				try {
					onCompletion.success(obj.getString("id"), obj.getString("rev"));
				} catch (JSONException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					onCompletion.error(e.getLocalizedMessage());
				}
			}

			@Override
			public void onError(String msg) {
				onCompletion.error(msg);
				Log.e(TAG, msg);
			}

			@Override
			public void onServiceUnavailable(String msg) {
				onCompletion.serviceUnavailable(msg);
			}
		};
        couchPost(dbUrl, doc, authToken, latestProc);
        return true;
    }

}

