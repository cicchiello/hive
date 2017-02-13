package com.jfc.misc.prop;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.MainActivity;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.PushEmbed;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;


public class AudioSampler {
	private static final String TAG = AudioSampler.class.getName();

	private Activity mActivity;
	private AlertDialog mAlert;
	private ProgressDialog mProgressDialog;
	private int mUpdateInterval_ms, mProgress;
	private Handler mProgressUpdateTimer = null;

	public AudioSampler(Activity _activity, ImageButton sampleButton) {
		mActivity = _activity;
		
	    File storageDir = null;
	    if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
	        //RUNTIME PERMISSION Android M
	        if(PackageManager.PERMISSION_GRANTED==mActivity.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)){
	            storageDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), "myPhoto");
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

		        final ImageView recordIv = (ImageView) mAlert.findViewById(R.id.record_button);
		        recordIv.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						String hiveId = HiveEnv.getHiveAddress(mActivity, ActiveHiveProperty.getActiveHiveProperty(mActivity));
						String sensor = "mic";
						String durationMsStr = Integer.toString(1000*10);
						String msg = "tx|"+hiveId.replace('-', ':')+"|action|"+sensor+"|"+durationMsStr;
						
						new PushEmbed(msg);
					}
				});
		        
		        final Runnable cancelAction = new Runnable() {
					@Override
					public void run() {mAlert.dismiss(); mAlert = null;}
				};
				
		        ImageView playIv = (ImageView) mAlert.findViewById(R.id.play_button);
		        playIv.setImageResource(R.drawable.ic_play_disabled);
		        playIv.setEnabled(false);
		        ImageView pauseIv = (ImageView) mAlert.findViewById(R.id.pause_button);
		        pauseIv.setImageResource(R.drawable.ic_pause_disabled);
		        pauseIv.setEnabled(false);
			}
		};
		sampleButton.setOnClickListener(ocl);
		
	}
	
	public void onPause() {
		if (mAlert != null) 
			mAlert.dismiss();
	}
}
