package com.jfc.srvc.cloud;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.util.Log;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;

public class CouchCmdPush {
	private static final String TAG = CouchCmdPush.class.getName();

	private Context mCtxt;
	private String mSensorName, mInstruction;
	private boolean mTruncate;
	private OnCompletion mOnCompletion;
	
	public interface OnCompletion {
		public void success();
		public void error(String query, String msg);
		public void serviceUnavailable(String msg);
	}
	
	public CouchCmdPush(Context ctxt, String sensorName, String instruction, OnCompletion onCompletion) {
		mCtxt = ctxt;
		mSensorName = sensorName;
		mInstruction = instruction;
		mOnCompletion = onCompletion;
		mTruncate = false;
	}

	public CouchCmdPush(Context ctxt, String sensorName, String instruction, boolean truncate, OnCompletion onCompletion) {
		mCtxt = ctxt;
		mSensorName = sensorName;
		mInstruction = instruction;
		mOnCompletion = onCompletion;
		mTruncate = truncate;
	}

	private void createNewMsgDoc(final String channelDocId, final String channelDocRev, final String prevId) {
		try {
			// create a new msg doc
			final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(mCtxt);
			final String authToken = DbCredentialsProperty.getAuthToken(mCtxt);
			final String payload = mSensorName + "|" + mInstruction;
	
			JSONObject msgDoc = new JSONObject();
			msgDoc.put("prev-msg-id", mTruncate ? "0" : prevId);
			msgDoc.put("payload", payload);
			msgDoc.put("timestamp", Long.toString(System.currentTimeMillis()/1000));
			
		    CouchPostBackground.OnCompletion postOnCompletion = new CouchPostBackground.OnCompletion() {
		    	public void onSuccess(String msgId, String msgRev) {
		    		try {
						JSONObject newChannelDoc = new JSONObject();
						if (channelDocRev != null)
							newChannelDoc.put("_rev", channelDocRev);
						newChannelDoc.put("msg-id", msgId);
						newChannelDoc.put("prev-msg-id", mTruncate ? "0" : prevId);
						newChannelDoc.put("payload", payload);
						newChannelDoc.put("timestamp", Long.toString(System.currentTimeMillis()/1000));
		
						// log the command
						String rpt = dbUrl+"/"+channelDocId+" "+newChannelDoc.toString();
						//ACRA.getErrorReporter().handleSilentException(new Exception("About to issue query: "+rpt));
						
						CouchPutBackground.OnCompletion putOnCompletion = new CouchPutBackground.OnCompletion() {
					    	public void complete(JSONObject results) {
					    		Log.i(TAG, "Channel Doc PUT success:  "+results.toString());
					    		if (mOnCompletion != null)
					    			mOnCompletion.success();
					    	}
					    	public void failed(String query, String msg) {
					    		Log.e(TAG, "Channel Doc PUT failed: "+msg);
					    		if (mOnCompletion != null)
					    			mOnCompletion.error(query, msg);
					    	}
						};
						
			    	    new CouchPutBackground(dbUrl+"/"+channelDocId, authToken, newChannelDoc.toString(), putOnCompletion).execute();
					} catch (JSONException je) {
						Log.e(TAG, je.getMessage());
					}
		    	}
		    	public void onFailure(String query, String msg) {
		    		Log.e(TAG, "Msg Doc POST failed: "+msg);
					ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
		    	}
		    };
		    new CouchPostBackground(dbUrl, authToken, msgDoc.toString(), postOnCompletion).execute();
		} catch (JSONException je) {
			Log.e(TAG, je.getMessage());
		}
	}
	
	
	public void execute() {
		String ActiveHive = ActiveHiveProperty.getActiveHiveName(mCtxt);
		String HiveId = HiveEnv.getHiveAddress(mCtxt, ActiveHive);
		final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(mCtxt);
		String authToken = null;
		final String channelDocId = HiveId + "-app";
		
	    CouchGetBackground.OnCompletion channelDocOnCompletion = new CouchGetBackground.OnCompletion() {
			@Override
	    	public void complete(JSONObject currentChannelDoc) {
				try {
					String prevMsgId = currentChannelDoc.getString("msg-id");
					final String currentChannelDocRev = currentChannelDoc.getString("_rev");
					
					createNewMsgDoc(channelDocId, currentChannelDocRev, prevMsgId);
				} catch (JSONException je) {
					Log.e(TAG, je.getMessage());
				}
			}
			
			@Override
			public void objNotFound(String query) {
				createNewMsgDoc(channelDocId, null, "0");
			}
			
			@Override
	    	public void failed(String query, String msg) {
				Log.i(TAG, "Channel Doc GET failed: "+msg);
				ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
			}
	    };

    	final CouchGetBackground getter = new CouchGetBackground(dbUrl+"/"+channelDocId, authToken, channelDocOnCompletion);
    	getter.execute();
	}

}
