package com.jfc.misc.prop;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.PollSensorBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.support.v4.content.FileProvider;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;


public class AudioSampler {
	private static final String TAG = AudioSampler.class.getName();

	private static final String AUDIO_OBJID_PROPERTY = "AUDIO_OBJID_PROPERTY";
	private static final String AUDIO_REQUEST_TIMESTAMP_PROPERTY = "AUDIO_REQUEST_TIMESTAMP_PROPERTY";
	private static final String AUDIO_ATTACHMENT_PROPERTY = "AUDIO_ATTACHMENT_PROPERTY";
	private static final String AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY = "AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY";
	private static final String DEFAULT_AUDIO_OBJID = "<unknown>";
	private static final String DEFAULT_AUDIO_ATTACHMENT = "<unknown>";
	private static final String DEFAULT_AUDIO_REQUEST_TIMESTAMP = "0";
	private static final String DEFAULT_AUDIO_ATTACHMENT_TIMESTAMP = "0";
	
	private AtomicBoolean mDownloadDone = new AtomicBoolean(false);
	private Runnable mDownloader = null;
	private ProgressDialog mProgress;
	private Runnable mPoller = null;
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	
	private Activity mActivity;
	private AlertDialog mAlert;
	private Thread mPollerThread = null;
	private DbAlertHandler mDbAlert = null;
	private TextView mAudioTV = null;
	private String mHiveId;
	private ExclusiveButtonWrapper mSampleButton;
	
	public AudioSampler(Activity _activity, final String hiveId, ImageButton _sampleButton, TextView _audioTV, DbAlertHandler _dbAlert) {
		mSampleButton = new ExclusiveButtonWrapper(_sampleButton, "audio");
		mActivity = _activity;
		mDbAlert = _dbAlert;
		mAudioTV = _audioTV;
		mHiveId = hiveId;
		
		View.OnClickListener ocl = new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mAlert == null) {
					AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
					
					builder.setIcon(R.drawable.ic_hive);
					builder.setView(R.layout.audio_dialog);
					builder.setTitle(R.string.audio_dialog_title);
			        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
			            @Override
			            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
			        });
			        mAlert = builder.show();
	
			        setRecordingState(false);
					setPlaybackState(getAttName(mActivity, hiveId));
				}
			}
		};
		
		mSampleButton.setOnClickListener(ocl);
	}

	public void pollCloud() {
		if (mActivity == null) {
			Log.i(TAG, "Stop here");
		}
		PollSensorBackground.OnSaveValue onSaveValue = new PollSensorBackground.OnSaveValue() {
			@Override
			public void save(final Activity activity, final String objId, String value, final long timestamp) {
				CouchGetBackground.OnCompletion displayer = new CouchGetBackground.OnCompletion() {
					public void complete(JSONObject results) {
						try {
							if (results.has("_attachments")) {
								String attName = results.getJSONObject("_attachments").keys().next();
								AudioSampler.setAttachment(activity, mHiveId, objId, attName, timestamp);
							}
							updateParentUI();
						} catch (JSONException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}
					public void objNotFound(String query) {failed(query, "Object Not Found (listener)");}
					public void failed(String query, String msg) {
						Toast.makeText(mActivity, msg+"; sending a report to my developer", Toast.LENGTH_LONG).show();
						ACRA.getErrorReporter().handleException(new Exception(msg));												}
				};
				final String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(activity);
				String authToken = null;
				new CouchGetBackground(dbUrl+"/"+objId, authToken, displayer).execute();
			}
		};
		String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(mActivity);
		PollSensorBackground.ResultCallback onCompletion = PollSensorBackground.getSensorOnCompletion(mActivity, 
				"listener", 0 /*R.id.audioSampleText*/, mDbAlert, onSaveValue);
        new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(mHiveId, "listener"), onCompletion).execute();
	}
	
	static private String uniqueIdentifier(String base, String hiveId) {
		return base + "|" + hiveId;
	}
	

	public static boolean isAudioPropertyDefined(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if ((SP.contains(uniqueIdentifier(AUDIO_OBJID_PROPERTY,hiveId)) &&
			 !SP.getString(uniqueIdentifier(AUDIO_OBJID_PROPERTY,hiveId), DEFAULT_AUDIO_OBJID).equals(DEFAULT_AUDIO_OBJID)) ||
			(SP.contains(uniqueIdentifier(AUDIO_ATTACHMENT_PROPERTY,hiveId)) &&
			 !SP.getString(uniqueIdentifier(AUDIO_ATTACHMENT_PROPERTY,hiveId), DEFAULT_AUDIO_ATTACHMENT).equals(DEFAULT_AUDIO_ATTACHMENT)) ||
			(SP.contains(uniqueIdentifier(AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY,hiveId)) &&
			 !SP.getString(uniqueIdentifier(AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY,hiveId), DEFAULT_AUDIO_ATTACHMENT_TIMESTAMP).equals(DEFAULT_AUDIO_ATTACHMENT_TIMESTAMP)) ||
			(SP.contains(uniqueIdentifier(AUDIO_REQUEST_TIMESTAMP_PROPERTY,hiveId)) &&
			 !SP.getString(uniqueIdentifier(AUDIO_REQUEST_TIMESTAMP_PROPERTY,hiveId), DEFAULT_AUDIO_REQUEST_TIMESTAMP).equals(DEFAULT_AUDIO_REQUEST_TIMESTAMP))) {
			return true;
		} else {
			return false;
		}
	}
	
	public static String getAttName(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		return SP.getString(uniqueIdentifier(AUDIO_ATTACHMENT_PROPERTY,hiveId), DEFAULT_AUDIO_ATTACHMENT);
	}
	
	public static String getObjId(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		return SP.getString(uniqueIdentifier(AUDIO_OBJID_PROPERTY,hiveId), DEFAULT_AUDIO_OBJID);
	}
	
	public static long getReqTimestamp(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(uniqueIdentifier(AUDIO_REQUEST_TIMESTAMP_PROPERTY,hiveId), DEFAULT_AUDIO_REQUEST_TIMESTAMP);
		return Long.parseLong(v);
	}

	public static long getTimestamp(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(uniqueIdentifier(AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY,hiveId), DEFAULT_AUDIO_ATTACHMENT_TIMESTAMP);
		return Long.parseLong(v);
	}

	public static void setRequestTimestamp(Context ctxt, String hiveId, long timestamp) {
		String timestampStr = Long.toString(timestamp);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(uniqueIdentifier(AUDIO_REQUEST_TIMESTAMP_PROPERTY,hiveId), DEFAULT_AUDIO_REQUEST_TIMESTAMP).equals(timestampStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(AUDIO_REQUEST_TIMESTAMP_PROPERTY,hiveId), timestampStr);
			editor.commit();
		}
	}
	
	public static void setAttachment(Context ctxt, String hiveId, String objId, String attName, long timestamp) {
		String timestampStr = Long.toString(timestamp);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(uniqueIdentifier(AUDIO_OBJID_PROPERTY,hiveId), DEFAULT_AUDIO_OBJID).equals(objId)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(AUDIO_OBJID_PROPERTY,hiveId), objId);
			editor.commit();
		}
		if (!SP.getString(uniqueIdentifier(AUDIO_ATTACHMENT_PROPERTY,hiveId), DEFAULT_AUDIO_ATTACHMENT).equals(attName)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(AUDIO_ATTACHMENT_PROPERTY,hiveId), attName);
			editor.commit();
		}
		if (!SP.getString(uniqueIdentifier(AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY,hiveId), DEFAULT_AUDIO_ATTACHMENT_TIMESTAMP).equals(timestampStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(AUDIO_ATTACHMENT_TIMESTAMP_PROPERTY,hiveId), timestampStr);
			editor.commit();
		}
	}

	public void updateParentUI() {
		if (AudioSampler.isAudioPropertyDefined(mActivity, mHiveId)) {
			final String attName = AudioSampler.getAttName(mActivity, mHiveId);
			String timestampStr = attName.substring(attName.indexOf('_')+1,attName.indexOf('.'));
			final CharSequence since = DateUtils.getRelativeTimeSpanString(Long.parseLong(timestampStr)*1000,
									System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS, DateUtils.FORMAT_ABBREV_RELATIVE);
			mActivity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
		    		TextView audioTimestampTv = (TextView) mActivity.findViewById(R.id.audioSampleTimestampText);
					Calendar cal = Calendar.getInstance(Locale.ENGLISH);
					cal.setTimeInMillis(System.currentTimeMillis());
					final String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
					if (!timestampStr.equals(audioTimestampTv.getText().toString())) {
						audioTimestampTv.setText(timestampStr);
						SplashyText.highlightModifiedField(mActivity, audioTimestampTv);
					}

				    // if there's no audio clip in flight, then enable the button and attach a click listener
					long lastRequestTimestamp = getReqTimestamp(mActivity, mHiveId);
					long lastAttachmentTimestamp = getTimestamp(mActivity, mHiveId);
		    		String description = null;
					if (lastAttachmentTimestamp > lastRequestTimestamp ||
						System.currentTimeMillis() > (lastRequestTimestamp + 4*60)*1000) {
						mSampleButton.enableButton();
						description = since.toString();
					} else {
						mSampleButton.disableButton();
						description = "working";
					}
		    		if (!description.equals(mAudioTV.getText().toString())) {
		    			mAudioTV.setText(description);
						SplashyText.highlightModifiedField(mActivity, mAudioTV);
		    		}
				}
			});
		}
	}
	

	private void downloadAndShareClip(final String objId, final String attName) {
		mDownloader = new Runnable() {
			@Override
			public void run() {
				InputStream in = null;
				FileOutputStream fos = null;
				try {
					try {
						String urlStr = DbCredentialsProperty.getCouchLogDbUrl(mActivity) + "/" +objId + "/" + attName;
						in = new BufferedInputStream(new URL(urlStr).openStream());
						File dir = new File(mActivity.getFilesDir(), "audio-clips");
						if (!dir.isDirectory()) 
							dir.mkdir();
						File downloadFile = new File(dir, attName);
						if (!downloadFile.exists()) {
							fos = new FileOutputStream(downloadFile);
							byte[] buf = new byte[1024];
							int n = 0;
							while (-1!=(n=in.read(buf))) {
							   fos.write(buf, 0, n);
							}
							fos.close();
							in.close();
							fos = null;
							in = null;
						}
						
						Intent sharingIntent = new Intent(android.content.Intent.ACTION_SEND);
						Uri uri = FileProvider.getUriForFile(mActivity, "com.jfc.apps.hive.fileprovider", downloadFile);
						sharingIntent.setType("audio/wav");
						sharingIntent.putExtra(Intent.EXTRA_STREAM, uri);
						sharingIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
						mActivity.startActivity(Intent.createChooser(sharingIntent, "Share Captured Clip"));			
					} catch (MalformedURLException e) {
						// TODO Auto-generated catch block
						Toast.makeText(mActivity, "Couldn't share clip: "+e.getMessage(), Toast.LENGTH_LONG).show();
					} catch (IOException e) {
						// TODO Auto-generated catch block
						Toast.makeText(mActivity, "Couldn't share clip: "+e.getMessage(), Toast.LENGTH_LONG).show();
					} finally {
						if (in != null) 
							in.close();
						if (fos != null)
							fos.close();
					}
				} catch (Exception e) {
					Log.e(TAG, e.getMessage());
				}
				mDownloadDone.set(true);
				mDownloader = null;
			}
		};

		Thread downloadThread = new Thread(mDownloader, "clip-download");
		mDownloadDone.set(false);
		downloadThread.start();
	}

	
	private void setRecordingState(boolean isRecording) {
		if (mAlert != null) {
	        final ImageView recordIv = (ImageView) mAlert.findViewById(R.id.record_button);
			if (isRecording && recordIv != null) {
				recordIv.setImageResource(R.drawable.recording);
				
				mProgress = new ProgressDialog(mAlert.getContext());
				mProgress.setTitle("Recording and Uploading...");
				mProgress.setMessage(mActivity.getText(R.string.workingOnRecording));
				mProgress.setCancelable(false); // disable dismiss by tapping outside of the dialog
				mProgress.show();
			} else {
				if (mProgress != null) {
					mProgress.dismiss();
					mProgress = null;
				}
		        
				recordIv.setImageResource(R.drawable.record);
		        recordIv.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						final long requestTimestamp = System.currentTimeMillis()/1000;
						final String proposedAttName = "LISTEN_"+requestTimestamp+".WAV";
		        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
							@Override
							public void success() {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										setRecordingState(true);
										setRequestTimestamp(mActivity, mHiveId, requestTimestamp);
										mSampleButton.disableButton();
										mAudioTV.setText("working");
										waitForPlayback(20, proposedAttName);
									}
								});
							}
							@Override
							public void error(String query, final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Runnable cancelAction = new Runnable() {public void run() {mAlert.dismiss(); mAlert = null;}};
										mAlert = DialogUtils.createAndShowErrorDialog(mActivity, msg, android.R.string.cancel, cancelAction);
									}
								});
								ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
							}
							@Override
							public void serviceUnavailable(final String msg) {mDbAlert.informDbInaccessible(mActivity, msg, 0);}
						};
		
						new CouchCmdPush(mActivity, "listen-ctrl", "start|20|"+proposedAttName, onCompletion).execute();
					}
				});
			}
		}
	}

	
	private void setPlaybackState(final String attName) {
		if (mAlert != null) {
			ImageView playIv = (ImageView) mAlert.findViewById(R.id.play_button);
			ImageView shareIv = (ImageView) mAlert.findViewById(R.id.share_button);
			TextView recordingTimestampTv = (TextView) mAlert.findViewById(R.id.recordingTimestamp);
			if (attName != null && !attName.equals(DEFAULT_AUDIO_ATTACHMENT)) {
				shareIv.setImageResource(R.drawable.ic_action_share);
				playIv.setImageResource(R.drawable.ic_play);
				Calendar cal = Calendar.getInstance(Locale.ENGLISH);
				cal.setTimeInMillis(Long.parseLong(attName.substring(attName.indexOf('_')+1,attName.indexOf('.')))*1000);
				String recordingTimestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
				String prevTimestampStr = recordingTimestampTv.getText().toString();
				if (!prevTimestampStr.equals(recordingTimestampStr)) {
					recordingTimestampTv.setText(recordingTimestampStr);
					SplashyText.highlightModifiedField(mActivity, recordingTimestampTv);
				} else {
					Log.i("TAG", "leaving the field unchanged");
				}
				final String objId = getObjId(mActivity, mHiveId);
				View.OnClickListener ocl = new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						Intent intent = new Intent(android.content.Intent.ACTION_VIEW);
						String urlStr = DbCredentialsProperty.getCouchLogDbUrl(mActivity) + "/" +objId + "/" + attName;
						intent.setDataAndType(Uri.parse(urlStr), "audio/wav");  
						mActivity.startActivity(intent);			
					}
				};
				playIv.setOnClickListener(ocl);
				ocl = new View.OnClickListener() {public void onClick(View v) {downloadAndShareClip(objId, attName);}};
				shareIv.setOnClickListener(ocl);
			} else {
				shareIv.setImageResource(R.drawable.ic_action_share_disabled);
				playIv.setImageResource(R.drawable.ic_play_disabled);
				recordingTimestampTv.setText("");
			}
		}
	}

	
	private void waitForPlayback(final int recordingDuration_s, final String attName) {
		final String startingAttName = getAttName(mActivity, mHiveId);
		mPoller = new Runnable() {
			@Override
			public void run() {
				// check about once a second whenever this activity is running
				int wavConversionDuration_s = (int)(17.0*recordingDuration_s/20.0+1.0);
				int uploadDuration_s = (int)(50.0*recordingDuration_s/20.0+1.0);
				int fudge_s = 40;
				int timeout = recordingDuration_s + wavConversionDuration_s + uploadDuration_s + fudge_s;
				boolean done = false;
				int pollCnt = 0;
				while (!mStopPoller.get() && !done) {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}

					pollCnt++;
					if (pollCnt % 5 == 0)
						  pollCloud();
					
					if (!mStopPoller.get())
						done = !startingAttName.equals(getAttName(mActivity, mHiveId)) || (timeout-- <= 0);
				}
				final boolean donetest = done;
				final boolean stoptest = mStopPoller.get();
				mActivity.runOnUiThread(new Runnable() {
					public void run() {
						boolean donetest2 = donetest;
						boolean stoptest2 = stoptest;
						String att = getAttName(mActivity, mHiveId);
						setPlaybackState(att);
						setRecordingState(false);
					}
				});
			}
		};
		mStopPoller.set(false);
		mPollerThread = new Thread(mPoller, "ListenPoller");
		mPollerThread.start();
	}
	
	public void onPause() {
		mStopPoller.set(true);
		
		if (mAlert != null) 
			mAlert.dismiss();
		
		if (mProgress != null) 
			mProgress.dismiss();

		mAlert = null;
		mProgress = null;
	}
}
