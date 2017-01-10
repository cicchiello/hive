package com.jfc.srvc.ble2cld;

import android.os.AsyncTask;
import android.util.Log;

import org.apache.http.client.ResponseHandler;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;

/**
 * Created by Joe on 12/12/2016.
 */

public class PostActuatorBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = PostActuatorBackground.class.getName();

    private String dbHost, db;
    private JSONObject doc;
    private int dbPort;
    private String docId, rev;

    public interface ResultCallback {
    	public void success(String id, String rev);
    	public void error(String msg);
    };
    
    public PostActuatorBackground(String _dbHost, int _dbPort, String _db, JSONObject _doc, ResultCallback _onCompletion) {
        dbHost = _dbHost;
        dbPort = _dbPort;
        db = _db;
        doc = _doc;
    }

    public String getDocId() {return docId;}
    public String getRev() {return rev;}
    
    interface ProcessResult {
    	public void onSuccess(JSONObject obj);
    	public void onError(String msg);
    };
    
    private static void couchPost(String dbHost, int dbPort, String db, JSONObject doc, String authToken, ProcessResult proc) {
        String urlStub = "http://"+dbHost+":"+dbPort+"/"+db;

        DefaultHttpClient httpclient;
        HttpPost httpPost;
        try {
            httpclient = new DefaultHttpClient();
            httpPost = new HttpPost(urlStub);
            StringEntity se = new StringEntity(doc.toString());
            httpPost.setEntity(se);
            httpPost.setHeader("Accept", "application/json");
            httpPost.setHeader("Content-type", "application/json");
            
            //Handles what is returned from the page 
            ResponseHandler responseHandler = new BasicResponseHandler();
            Object o = httpclient.execute(httpPost, responseHandler);
            
            Log.i(TAG, o.toString());
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
        }
    }
    
    @Override
    protected Boolean doInBackground(Void... params) {
        final String authToken = null;
        ProcessResult latestProc = new ProcessResult() {

			@Override
			public void onSuccess(JSONObject obj) {
				Log.i(TAG, "have success");
			}

			@Override
			public void onError(String msg) {
				Log.e(TAG, msg);
			}
		};
        couchPost(dbHost, dbPort, db, doc, authToken, latestProc);
        return true;
    }

}

