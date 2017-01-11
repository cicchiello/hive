package com.jfc.srvc.ble2cld;

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

/**
 * Created by Joe on 12/12/2016.
 */

public class CouchPostBackground extends AsyncTask<Void,Void,Boolean> {
    private static final String TAG = CouchPostBackground.class.getName();

    private String doc, dbUrl, authToken, devName;
    private String docId, rev;
    private CmdOnCompletion onCompletion;

    public CouchPostBackground(String _dbUrl, String _authToken, String _devName, String _doc, CmdOnCompletion _onCompletion) {
    	dbUrl = _dbUrl;
    	authToken = _authToken;
        doc = _doc;
        devName = _devName;
        onCompletion = _onCompletion;
    }

    public String getDocId() {return docId;}
    public String getRev() {return rev;}
    public String getDevName() {return devName;}

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
                System.err.println("CouchDb PUT of attachment mapper failed; HTTP error code : " + response.getStatusLine().getStatusCode());
                onCompletion.complete("rply|"+devName+"|POST|error");
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

                onCompletion.complete("rply|"+devName+"|POST|success");
                return true;
            }
            onCompletion.complete("rply|"+devName+"|POST|error");
        } catch (URISyntaxException e) {
            onCompletion.complete("rply|POST|error: "+"URISyntaxException: "+e);
            System.err.println("URISyntaxException: "+e);
        } catch (ClientProtocolException e) {
            onCompletion.complete("rply|POST|error: "+"ClientProtocolException: "+e);
            System.err.println("ClientProtocolException: "+e);
        } catch (IOException e) {
            onCompletion.complete("rply|POST|error: "+"IOException: "+e);
            System.err.println("IOException: "+e);
        } catch (JSONException e) {
            onCompletion.complete("rply|POST|error: "+"JSONException: "+e);
            System.err.println("JSONException: " + e);
        } catch (Exception e) {
            onCompletion.complete("rply|POST|error: "+"Exception: "+e);
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

