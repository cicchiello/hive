package com.jfc.srvc.cloud;

import android.os.AsyncTask;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
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

public class CouchPostBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = CouchPostBackground.class.getName();

    private String doc, dbUrl, authToken;
    private String docId, rev;
    private OnCompletion onCompletion;

    public interface OnCompletion {
    	public void onSuccess(String docId, String rev);
    	public void onFailure(String query, String msg);
    };
    
    public CouchPostBackground(String _dbUrl, String _authToken, String _doc, OnCompletion _onCompletion) {
    	dbUrl = _dbUrl;
    	authToken = _authToken;
        doc = _doc;
        onCompletion = _onCompletion;
    }

    public String getDocId() {return docId;}
    public String getRev() {return rev;}

    @Override
    protected Boolean doInBackground(Void... params) {
        String urlStub = dbUrl;

        InputStreamReader is = null;
        BufferedReader br = null;
        DefaultHttpClient client = null;
        try {
            HttpPost post= new HttpPost(new URI(urlStub));
            post.addHeader("Content-Type", "application/json");
            post.addHeader("Accept", "application/json");
            if (authToken != null)
                post.addHeader("Authorization", "Basic "+authToken);

            post.setEntity(new StringEntity(doc));

            client = new DefaultHttpClient();
            HttpResponse response = client.execute(post);

            //System.err.println("Response: "+response.getStatusLine());
            if (response.getStatusLine().getStatusCode() != 201) {
                System.err.println("CouchDb POST of attachment mapper failed; HTTP error code : " + response.getStatusLine().getStatusCode());
                onCompletion.onFailure(urlStub, response.toString());
                return false;
            }

            br = new BufferedReader(is = new InputStreamReader(response.getEntity().getContent()));
            StringBuilder builder = new StringBuilder();
            for (String line = null; (line = br.readLine()) != null;)
                builder.append(line).append("\n");

            //System.err.println("Response: "+builder);

            String stat = new JSONObject(new JSONTokener(builder.toString())).getString("ok");
            if (stat.equalsIgnoreCase("true")) {
                docId = new JSONObject(new JSONTokener(builder.toString())).getString("id");
                rev = new JSONObject(new JSONTokener(builder.toString())).getString("rev");

                onCompletion.onSuccess(docId, rev);
                return true;
            }
            
            onCompletion.onFailure(urlStub, response.toString());
        } catch (URISyntaxException e) {
        	onCompletion.onFailure(urlStub, e.getMessage());
            System.err.println("URISyntaxException: "+e);
        } catch (ClientProtocolException e) {
        	onCompletion.onFailure(urlStub, e.getMessage());
            System.err.println("ClientProtocolException: "+e);
        } catch (UnknownHostException e) {
        	onCompletion.onFailure(urlStub, e.getMessage());
        	System.err.println("UnknownHostException: "+e);
        } catch (IOException e) {
        	onCompletion.onFailure(urlStub, e.getMessage());
            System.err.println("IOException: "+e);
        } catch (JSONException e) {
        	onCompletion.onFailure(urlStub, e.getMessage());
            System.err.println("JSONException: " + e);
        } catch (Exception e) {
        	onCompletion.onFailure(urlStub, e.getMessage());
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

