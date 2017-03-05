package com.jfc.misc.prop;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.util.Log;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.CouchPostBackground;
import com.jfc.srvc.cloud.CouchPutBackground;


public class PropertyBase {
	private static final String TAG = PropertyBase.class.getName();

	protected Activity mActivity;
	protected TextView mTimestampTV;
	protected TextView mValueTV;
	protected String mHiveId;
	protected ImageButton mButton;
	
	protected PropertyBase(Activity _activity, String _hiveId, TextView _valueTV, ImageButton _button, TextView _timestampTV) {
		this.mActivity = _activity;
		this.mValueTV = _valueTV;
		this.mTimestampTV = _timestampTV;
		this.mButton = _button;
		this.mHiveId = _hiveId;
	}

	protected void createNewMsgDoc(final String channelDocId, final String channelDocRev, String sensor, String instruction, String prevId) {
		try {
			// create a new msg doc
			final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(mActivity);
			final String authToken = DbCredentialsProperty.getAuthToken(mActivity);
			String timestamp = Long.toString(System.currentTimeMillis()/1000);
			String payload = sensor + "|" + instruction;
	
			JSONObject msgDoc = new JSONObject();
			msgDoc.put("prev-msg-id", prevId);
			msgDoc.put("payload", payload);
			msgDoc.put("timestamp", timestamp);
			
		    CouchPostBackground.OnCompletion postOnCompletion = new CouchPostBackground.OnCompletion() {
		    	public void onSuccess(String msgId, String msgRev) {
		    		try {
		    			String timestamp = Long.toString(System.currentTimeMillis()/1000);
					
						JSONObject newChannelDoc = new JSONObject();
						if (channelDocRev != null)
							newChannelDoc.put("_rev", channelDocRev);
						newChannelDoc.put("msg-id", msgId);
						newChannelDoc.put("timestamp", timestamp);
		
						CouchPutBackground.OnCompletion putOnCompletion = new CouchPutBackground.OnCompletion() {
					    	public void complete(JSONObject results) {
					    		Log.i(TAG, "Channel Doc PUT success:  "+results.toString());
					    	}
					    	public void failed(String msg) {
					    		Log.e(TAG, "Channel Doc PUT failed: "+msg);
					    	}
						};
						
			    	    new CouchPutBackground(dbUrl+"/"+channelDocId, authToken, newChannelDoc.toString(), putOnCompletion).execute();
					} catch (JSONException je) {
						Log.e(TAG, je.getMessage());
					}
		    	}
		    	public void onFailure(String msg) {
		    		Log.e(TAG, "Msg Doc POST failed: "+msg);
		    	}
		    };
		    new CouchPostBackground(dbUrl, authToken, msgDoc.toString(), postOnCompletion).execute();
		} catch (JSONException je) {
			Log.e(TAG, je.getMessage());
		}
	}
	
	
	protected void postToDb(final String sensor, final String instruction) {
		final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(mActivity);
		String authToken = null;
		final String channelDocId = mHiveId + "-app";
		
	    CouchGetBackground.OnCompletion channelDocOnCompletion = new CouchGetBackground.OnCompletion() {
			@Override
	    	public void complete(JSONObject currentChannelDoc) {
				try {
					String prevMsgId = currentChannelDoc.getString("msg-id");
					final String currentChannelDocRev = currentChannelDoc.getString("_rev");
					
					createNewMsgDoc(channelDocId, currentChannelDocRev, sensor, instruction, prevMsgId);
				} catch (JSONException je) {
					Log.e(TAG, je.getMessage());
				}
			}
			
			@Override
			public void objNotFound() {
				createNewMsgDoc(channelDocId, null, sensor, instruction, "0");
			}
			
			@Override
	    	public void failed(String msg) {
				Log.i(TAG, "Channel Doc GET failed: "+msg);
			}
	    };

    	final CouchGetBackground getter = new CouchGetBackground(dbUrl+"/"+channelDocId, authToken, channelDocOnCompletion);
    	getter.execute();
	}

	// derived class should do whatever is necessary to determine if the Property hasn't been defined or ever touched
	protected boolean isPropertyDefined() {return false;}
	
	// do whatever is necessary to display the undfined state in the ParentView
	protected void displayUndefined() {};
	
	// do whatever is necessary to display the current state in the ParentView
	protected void display() {}
	
	// do whatever is necessary to cleanup and/or save state when the owning activity is paused
	public void onPause() {}
	
}
