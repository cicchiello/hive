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

import com.jfc.apps.hive.MainActivity;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Environment;
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
	private File storageDir = null;
	private DbAlertHandler mDbAlert = null;
	private ImageButton mSampleButton = null;
	private String mHiveId;
	
	public AudioSampler(Activity _activity, final String hiveId, ImageButton _sampleButton, DbAlertHandler _dbAlert) {
		mActivity = _activity;
		mDbAlert = _dbAlert;
		mSampleButton = _sampleButton;
		mHiveId = hiveId;
		
	    if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
	        //RUNTIME PERMISSION Android M
	        if(PackageManager.PERMISSION_GRANTED==mActivity.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)){
	        	storageDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
	        }else{
	        	MainActivity m = (MainActivity) mActivity;
	        	m.requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
	        }    
	    }

		View.OnClickListener ocl = new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.audio_dialog);
				builder.setTitle(R.string.audio_dialog_title);
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();

				setPlaybackState(false, getAttName(mActivity, hiveId));
			}
		};
		
		mSampleButton.setOnClickListener(ocl);
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

	static public void updateParentUI(final Activity activity, final String hiveId, final int audioResid, 
									  final int audioTimestampResid, final int buttonResid) {
		if (AudioSampler.isAudioPropertyDefined(activity, hiveId)) {
			final String attName = AudioSampler.getAttName(activity, hiveId);
			String timestampStr = attName.substring(attName.indexOf('_')+1,attName.indexOf('.'));
			final CharSequence since = DateUtils.getRelativeTimeSpanString(Long.parseLong(timestampStr)*1000,
					System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS, DateUtils.FORMAT_ABBREV_RELATIVE);
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
		    		TextView audioTv = (TextView) activity.findViewById(audioResid);
		    		if (!since.equals(audioTv.getText().toString())) {
		    			audioTv.setText(since);
						SplashyText.highlightModifiedField(activity, audioTv);
		    		}
		    		TextView audioTimestampTv = (TextView) activity.findViewById(audioTimestampResid);
					Calendar cal = Calendar.getInstance(Locale.ENGLISH);
					cal.setTimeInMillis(System.currentTimeMillis());
					final String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
					if (!timestampStr.equals(audioTimestampTv.getText().toString())) {
						audioTimestampTv.setText(timestampStr);
						SplashyText.highlightModifiedField(activity, audioTimestampTv);
					}

				    // if there's no audio clip in flight, then enable the button and attach a click listener
					long lastRequestTimestamp = getReqTimestamp(activity, hiveId);
					long lastAttachmentTimestamp = getTimestamp(activity, hiveId);
		    		ImageButton sampleButton = (ImageButton) activity.findViewById(buttonResid);
					if (lastAttachmentTimestamp > lastRequestTimestamp ||
						System.currentTimeMillis() > (lastRequestTimestamp + 4*60)*1000) {
						sampleButton.setImageResource(R.drawable.ic_rarrow);
						sampleButton.setEnabled(true);
					} else {
						sampleButton.setImageResource(R.drawable.ic_rarrow_disabled);
						sampleButton.setEnabled(false);
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

	
	private void setPlaybackState(boolean isRecording, final String attName) {
		if (mAlert != null) {
	        final ImageView recordIv = (ImageView) mAlert.findViewById(R.id.record_button);
			ImageView playIv = (ImageView) mAlert.findViewById(R.id.play_button);
			ImageView shareIv = (ImageView) mAlert.findViewById(R.id.share_button);
			TextView recordingTimestampTv = (TextView) mAlert.findViewById(R.id.recordingTimestamp);
			if (isRecording) {
				recordIv.setImageResource(R.drawable.recording);
				
				mProgress = new ProgressDialog(mAlert.getContext());
				mProgress.setTitle("Recording and Uploading...");
				mProgress.setMessage("Working (will take over 2 minutes)...");
				mProgress.setCancelable(false); // disable dismiss by tapping outside of the dialog
				mProgress.show();
			} else {
				if (mProgress != null)
					mProgress.dismiss();
		        
				recordIv.setImageResource(R.drawable.record);
		        recordIv.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						final long requestTimestamp = System.currentTimeMillis()/1000;
						final String attName = "LISTEN_"+requestTimestamp+".WAV";
		        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
							@Override
							public void success() {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										setPlaybackState(true, attName);
										setRequestTimestamp(mActivity, mHiveId, requestTimestamp);
										mSampleButton.setImageResource(R.drawable.ic_rarrow_disabled);
										mSampleButton.setEnabled(false);
										waitForPlayback(20, attName);
									}
								});
							}
							@Override
							public void error(final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Runnable cancelAction = new Runnable() {public void run() {mAlert.dismiss(); mAlert = null;}};
										mAlert = DialogUtils.createAndShowErrorDialog(mActivity, msg, android.R.string.cancel, cancelAction);
									}
								});
							}
							@Override
							public void serviceUnavailable(final String msg) {mDbAlert.informDbInaccessible(mActivity, msg, 0);}
						};
		
						new CouchCmdPush(mActivity, "listen-ctrl", "start|20|"+attName, onCompletion).execute();
					}
				});
			}
		
			if (attName != null && !attName.equals(DEFAULT_AUDIO_ATTACHMENT)) {
				shareIv.setImageResource(R.drawable.ic_action_share);
				playIv.setImageResource(R.drawable.ic_play);
				Calendar cal = Calendar.getInstance(Locale.ENGLISH);
				cal.setTimeInMillis(Long.parseLong(attName.substring(attName.indexOf('_')+1,attName.indexOf('.')))*1000);
				String recordingTimestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
				if (!recordingTimestampTv.getText().toString().equals(recordingTimestampStr)) {
					recordingTimestampTv.setText(recordingTimestampStr);
					SplashyText.highlightModifiedField(mActivity, recordingTimestampTv);
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
				ocl = new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						downloadAndShareClip(objId, attName);
					}
				};
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
				int timeout = recordingDuration_s + recordingDuration_s*8;
				boolean done = false;
				while (!mStopPoller.get() && !done) {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					
					if (!mStopPoller.get())
						done = !startingAttName.equals(getAttName(mActivity, mHiveId)) || (timeout-- <= 0);
				}
				mActivity.runOnUiThread(new Runnable() {
					public void run() {setPlaybackState(false, getAttName(mActivity, mHiveId));}
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
