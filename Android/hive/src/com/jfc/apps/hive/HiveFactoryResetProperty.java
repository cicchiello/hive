package com.jfc.apps.hive;

import java.util.List;

import com.example.hive.R;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.util.misc.DialogUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;

public class HiveFactoryResetProperty implements IPropertyMgr {
	private static final String TAG = HiveFactoryResetProperty.class.getName();

	private AlertDialog alert;

	public interface Resetter {
		void reset(Activity activity);
	};
	
	public HiveFactoryResetProperty(final Activity activity, ImageButton button, final List<Resetter> resetters) {
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				
		    	Runnable scrubAction = new Runnable() {
		    		@Override
		    		public void run() {
		    			alert = null; 
		    			for (Resetter r : resetters) 
		    				r.reset(activity);
		    			activity.onBackPressed();
		    		}
		    	};
		    	Runnable cancelAction = new Runnable() {
					@Override
					public void run() {alert=null;}
		    	};
		    	alert = DialogUtils.createAndShowAlertDialog(activity, R.string.factory_reset_question, 
		    										    	 R.string.yes, scrubAction, R.string.cancel, cancelAction);
			}
		});
	}

	public AlertDialog getAlertDialog() {return alert;}
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {return false;}
	
	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {}
	
	@Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {}
	
	@Override
    public void onSaveInstanceState(Bundle savedInstanceState) {}
}
