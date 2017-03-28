package com.jfc.misc.prop;


//import android.app.Activity;
//import android.app.AlertDialog;
//import android.content.Context;
//import android.content.DialogInterface;
//import android.view.View;
//import android.view.View.OnClickListener;
//import android.widget.EditText;
//import android.widget.ImageButton;
//import android.widget.TextView;
//
//import com.jfc.apps.hive.HiveEnv;
//import com.jfc.apps.hive.R;
//import com.jfc.util.misc.SplashyText;
//
//
//public class PrescaleProperty extends IntProperty {
//	private static final String TAG = PrescaleProperty.class.getName();
//
//	private static final String PRESCALE_PROPNAME = "PRESCALE";
//	private static final int DEFAULT_PRESCALE = 121;
//	
//	private String mHiveId, mPropId;
//	
//	static private String uniqueIdentifier(String base, String hiveId) {
//		return base + "|" + hiveId;
//	}
//	
//	public PrescaleProperty(final Activity activity, final TextView tv, ImageButton button) {
//		super(activity, uniqueIdentifier(PRESCALE_PROPNAME, HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity))), DEFAULT_PRESCALE, tv, button);
//		this.mHiveId = HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity));
//		this.mPropId = uniqueIdentifier(PRESCALE_PROPNAME, mHiveId);
//		
//    	button.setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
//				
//				builder.setIcon(R.drawable.ic_hive);
//				builder.setView(R.layout.one_int_property_dialog);
//				builder.setTitle(R.string.steps_per_second_dialog_title);
//				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
//		        	@Override
//		        	public void onClick(DialogInterface dialog, int which) {
//		        		int newPrescale = Integer.parseInt(((EditText)mAlert.findViewById(R.id.stepsPerRevTextValue)).getText().toString());
//		        		setPrescale(newPrescale, mHiveId);
//		        		SplashyText.highlightModifiedField(mActivity, mTV);
//						mAlert.dismiss(); 
//						mAlert = null;
//		        	}
//		        });
//		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
//		            @Override
//		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
//		        });
//		        mAlert = builder.show();
//		        
//		        EditText tv = (EditText) mAlert.findViewById(R.id.stepsPerRevTextValue);
//		        int t = getPrescale(mActivity, mHiveId);
//		        tv.setText(Integer.toString(t));
//			}
//		});
//	}
//
//
//	public static boolean isPrescaleDefined(Context ctxt, String hiveId) {
//		return IntProperty.isDefined(ctxt, uniqueIdentifier(PRESCALE_PROPNAME, hiveId), DEFAULT_PRESCALE);
//	}
//	
//	public static int getPrescale(Context ctxt, String hiveId) {
//		return IntProperty.getVal(ctxt, uniqueIdentifier(PRESCALE_PROPNAME, hiveId), DEFAULT_PRESCALE);
//	}
//	
//	public static void resetPrescale(Context ctxt, String hiveId) {
//		IntProperty.reset(ctxt, uniqueIdentifier(PRESCALE_PROPNAME, hiveId), DEFAULT_PRESCALE);
//	}
//	
//	private void setPrescale(int steps, String hiveId) {
//		set(uniqueIdentifier(PRESCALE_PROPNAME, hiveId), steps, DEFAULT_PRESCALE);
//	}
//}
