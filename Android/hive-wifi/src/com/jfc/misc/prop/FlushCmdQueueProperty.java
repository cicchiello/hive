package com.jfc.misc.prop;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.text.format.DateFormat;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

public class FlushCmdQueueProperty implements IPropertyMgr {
	private static final String TAG = FlushCmdQueueProperty.class.getName();

	private Activity mActivity;
	private AlertDialog mAlert;
	private List<Pair<String,String>> mCmds = null;
	private ArrayAdapter<Pair<String,String>> mArrayAdapter = null;

	private void cancelAlert() {if (mAlert != null) mAlert.dismiss(); mAlert = null;}
	private final Runnable cancelAction = new Runnable() {public void run() {cancelAlert();}};

	public FlushCmdQueueProperty(Activity activity, final DbAlertHandler dbAlert, ImageButton button) {
		mActivity = activity;
    	mCmds = new ArrayList<Pair<String,String>>();
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
		        mArrayAdapter = new CmdQueueAdapter(mActivity, mCmds);
		        
		    	AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
		        builder.setIcon(R.drawable.ic_hive);
		        builder.setTitle(R.string.cmd_queue_title);
		        builder.setPositiveButton(R.string.clear, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
		        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
							@Override
							public void success() {
								cancelAlert();
								mAlert = DialogUtils.createAndShowAlertDialog(mActivity, "Queue cleared", 0, null, 0, null, android.R.string.ok, cancelAction);
							}
							@Override
							public void error(String query, final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									public void run() {cancelAlert(); mAlert = DialogUtils.createAndShowErrorDialog(mActivity, msg, android.R.string.cancel, cancelAction);}
								});
								ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
							}
							@Override
							public void serviceUnavailable(final String msg) {
								cancelAlert();
								dbAlert.informDbInaccessible(mActivity, msg, 0);
							}
						};
	
						boolean truncate = true;
						new CouchCmdPush(mActivity, "noop", "noop", truncate, onCompletion).execute();
						//Toast.makeText(mActivity, "here's where I'd push new records if I  was online", Toast.LENGTH_LONG).show();
					}
				});
		        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
		            public void onClick(DialogInterface dialog, int which) {cancelAlert();}
		        });
		        builder.setAdapter(mArrayAdapter, null);
		        
		        mAlert = builder.show();
		        getCmdQueue();
			}
		});
	}
	
	public AlertDialog getAlertDialog() {return mAlert;}
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {return false;}
	
	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {}
	
	public static class CmdQueueAdapter extends ArrayAdapter<Pair<String,String>> {
	    public CmdQueueAdapter(Context context, List<Pair<String,String>> cmds) {
	       super(context, 0, cmds);
	    }

	    @Override
	    public View getView(int position, View convertView, ViewGroup parent) {
	    	// Get the data item for this position
	    	Pair<String,String> pair = getItem(position); 
	    	
	    	// Check if an existing view is being reused, otherwise inflate the view
	    	if (convertView == null) {
	    		convertView = LayoutInflater.from(getContext()).inflate(R.layout.cmd_entry, parent, false);
	    	}
	    	
	    	// Lookup view for data population
	    	TextView cmdTimestampTV = (TextView) convertView.findViewById(R.id.cmdTimestamp);
	    	TextView cmdTV = (TextView) convertView.findViewById(R.id.cmd);
	    	
			Calendar cal = Calendar.getInstance(Locale.ENGLISH);
			long timestamp_s = Long.parseLong(pair.first);
			cal.setTimeInMillis(timestamp_s*1000);
			String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
			
	    	// Populate the data into the template view using the data object
	    	cmdTimestampTV.setText(timestampStr);
	    	cmdTV.setText(pair.second);
	    	
	    	// Return the completed view to render on screen
	    	return convertView;
	    }
	}

	class JSONOnCompletionProcessor implements CouchGetBackground.OnCompletion {
		private void reportError(final String query, final String msg) {
			mActivity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					Toast.makeText(mActivity, msg+"; sending a report to my developer", Toast.LENGTH_LONG).show();
					ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
				}
			});
		}

		protected void processDoc(JSONObject doc) throws JSONException {}
		
		@Override
		public void complete(JSONObject result) {
			try {
				processDoc(result);
			} catch (JSONException je) {
				failed("invald JSONDoc: "+result.toString(), je.getMessage());
			}
		}

		public void failed(final String query, final String msg) {reportError(query, msg);}
		public void objNotFound(String query) {failed(query, "Object Not Found");}
	};

	private void getCmdQueue() {
		getCmdQueueDb();
		//getCmdQueueTest();
	}
	
	@SuppressWarnings("unused")
	private void getCmdQueueTest() {
		Runnable simulator = new Runnable() {
			@Override
			public void run() {
				try {
					Thread.sleep(500);
					mActivity.runOnUiThread(new Runnable() {
						public void run() {
							mCmds.add(new Pair<String,String>("1488000000", "some-command"));
							mArrayAdapter.notifyDataSetChanged();
						}
					});
					Thread.sleep(1500);
					mActivity.runOnUiThread(new Runnable() {
						public void run() {
							mCmds.add(new Pair<String,String>("1487100000", "some-other"));
							mArrayAdapter.notifyDataSetChanged();
						}
					});
					Thread.sleep(15500);
					mActivity.runOnUiThread(new Runnable() {
						public void run() {
							mCmds.add(new Pair<String,String>("1487000000", "some-other"));
							mArrayAdapter.notifyDataSetChanged();
						}
					});
					for (int i = 0; i < 10; i++) {
						Thread.sleep(1500);
						final String cmd = "a command "+Integer.toString(i);
						mActivity.runOnUiThread(new Runnable() {
							public void run() {
								mCmds.add(new Pair<String,String>("1487000000", cmd));
								mArrayAdapter.notifyDataSetChanged();
							}
						});
					}
				} catch (Exception e) {}
			}
		};
		new Thread(simulator).start();
	}
	
    private void getCmdQueueDb() {
		final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(mActivity);
		final String authToken = null;

		final JSONOnCompletionProcessor payloadOnCompletion = new JSONOnCompletionProcessor() {
			protected void processDoc(final JSONObject cmdDoc) throws JSONException {
				String ts = cmdDoc.getString("timestamp");
				String payload = cmdDoc.getString("payload");
				if (!"noop|noop".equals(payload)) {
					final Pair<String,String> p = new Pair<String,String>(ts,payload);
					mActivity.runOnUiThread(new Runnable() {
						public void run() {
							mCmds.add(p);
							mArrayAdapter.notifyDataSetChanged();
						}
					});
			    	new CouchGetBackground(dbUrl+"/"+cmdDoc.getString("prev-msg-id"), authToken, this).execute();
				}
			}
		};

		JSONOnCompletionProcessor headerOnCompletion = new JSONOnCompletionProcessor() {
			protected void processDoc(JSONObject headerDoc) throws JSONException {
		    	new CouchGetBackground(dbUrl+"/"+headerDoc.getString("msg-id"), authToken, payloadOnCompletion).execute();
			}
		};
    	
		String hiveId = HiveEnv.getHiveAddress(mActivity, ActiveHiveProperty.getActiveHiveName(mActivity));
    	new CouchGetBackground(dbUrl+"/"+hiveId+"-app", authToken, headerOnCompletion).execute();
    }
}
