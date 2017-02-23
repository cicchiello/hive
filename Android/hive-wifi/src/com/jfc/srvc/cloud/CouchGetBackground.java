package com.jfc.srvc.cloud;

import android.os.AsyncTask;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.UnknownHostException;

/**
 * Created by Joe on 12/12/2016.
 */

public class CouchGetBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = CouchGetBackground.class.getName();

    private String dbUrl, authToken;
    private OnCompletion onCompletion;

    public interface OnCompletion {
    	public void complete(JSONObject results);
    	public void failed(String msg);
    };
    
    public CouchGetBackground(String _dbUrl, String _authToken, OnCompletion _onCompletion) {
    	dbUrl = _dbUrl;
    	authToken = _authToken;
        onCompletion = _onCompletion;
    }

    @Override
    protected Boolean doInBackground(Void... params) {
        String urlStub = dbUrl;

        InputStreamReader is = null;
        BufferedReader br = null;
        DefaultHttpClient client = null;
        try {
            HttpGet getter= new HttpGet(new URI(urlStub));
            if (authToken != null)
            	getter.addHeader("Authorization", "Basic "+authToken);

            client = new DefaultHttpClient();
            HttpResponse response = client.execute(getter);

            //System.err.println("Response: "+response.getStatusLine());
            if (response.getStatusLine().getStatusCode() != 200) {
                System.err.println("CouchDb GET failed; HTTP error code : " + response.getStatusLine().getStatusCode());
                onCompletion.failed(response.getStatusLine().toString());
                return false;
            }

            br = new BufferedReader(is = new InputStreamReader(response.getEntity().getContent()));
            StringBuilder builder = new StringBuilder();
            for (String line = null; (line = br.readLine()) != null;)
                builder.append(line).append("\n");

            //System.err.println("Response: "+builder);

            onCompletion.complete(new JSONObject(new JSONTokener(builder.toString())));
        } catch (URISyntaxException e) {
        	onCompletion.failed(e.getMessage());
            System.err.println("URISyntaxException: "+e);
        } catch (ClientProtocolException e) {
        	onCompletion.failed(e.getMessage());
            System.err.println("ClientProtocolException: "+e);
        } catch (UnknownHostException e) {
        	onCompletion.failed(e.getMessage());
        	System.err.println("UnknownHostException: "+e);
        } catch (IOException e) {
        	onCompletion.failed(e.getMessage());
            System.err.println("IOException: "+e);
        } catch (JSONException e) {
        	onCompletion.failed(e.getMessage());
            System.err.println("JSONException: " + e);
        } catch (Exception e) {
        	onCompletion.failed(e.getMessage());
            System.err.println("Exception: " + e);
        } finally {
            try {
                if (client != null) {
                    try {
                        Method m = client.getClass().getMethod("close");
                        if (m != null) m.invoke(client);
                    } catch (Exception e) {
                        // nothing I can do if there's a problem here...
                    }
                }
                if (br != null) br.close();
                if (is != null) is.close();
            } catch (IOException ioe) {
                // ignore
            }
            br = null;
            is = null;
        }
        return false;
    }

}

