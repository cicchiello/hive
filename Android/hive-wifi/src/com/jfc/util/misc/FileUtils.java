package com.jfc.util.misc;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Calendar;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

public class FileUtils {
	private static final String TAG = FileUtils.class.getSimpleName();


//	Place this in an activity before attempting to use the file system
//    File storageDir = null;
//    if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
//        //RUNTIME PERMISSION Android M
//        if(PackageManager.PERMISSION_GRANTED==checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)){
//            storageDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), "myPhoto");
//        }else{
//            requestPermission();
//        }    
//    } 
//
//	// then do something like the following once a file is needed
//	try {
//		String SDCardRoot = Environment.getExternalStorageDirectory().getAbsolutePath() + "/";
//	    File dirFile = new File(SDCardRoot + "hive" + File.separator);
//	    boolean stat1 = dirFile.mkdirs();
//	    File file = new File(SDCardRoot + "hive" + File.separator + "log.log");
//	    boolean stat = file.createNewFile();
//	    
////			String publicTempfilePath = FileUtils.getCacheDir(getApplicationContext(),"log") + File.separator + "log.log";
//			OutputStream os = new FileOutputStream(file);
//			BleGattExecutor.setOutputStream(os);
//	} catch (Exception e) {
//		Log.e(TAG, "Exception!?!?");
//	}
		

	private static StorageOptions storage;
	private synchronized static StorageOptions getStorageOptions(Context ctxt) {
		if (storage == null) 
			storage = new StorageOptions(ctxt);
		return storage;
	}

	public static File getCacheDir(Context ctxt, String leafFoldername) {
		// it turns out that finding the sdcard is surprisingly difficult/inconsistent between Android devices
		// ...  so, first use the provided functions that work on older devices.  Then, try the heuristic approach
		// found online (see StorageOptions) to override if it finds a valid result.

		// SD Card Mounted
		File cacheDir;
	    if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
	    	cacheDir = Environment.getExternalStorageDirectory();
	    } else {
	    	// Use internal storage
	    	cacheDir = ctxt.getCacheDir();
	    }

		StorageOptions storage = getStorageOptions(ctxt);
		boolean foundGoodCandidate = false;
		for (int i = 0; i < storage.numDevices() && !foundGoodCandidate; i++) {
			if (storage.getDeviceType(i) == StorageOptions.Type.external) {
				foundGoodCandidate = true;
				cacheDir = new File(storage.getDevicePath(i));
			}
		}
		
	    cacheDir = new File(cacheDir.getAbsolutePath()+File.separator + leafFoldername);
	    
	    // Create the cache directory if it doesn't exist
	    if (!cacheDir.exists()) {
	    	boolean stat = cacheDir.mkdirs();
	    	Log.i(TAG, "stat: "+stat);
	    }
	    
	    return cacheDir;
	}
	
	public static String createTimestamp(boolean includeDate, boolean includeHourAndMinute, boolean includeSeconds) {
        Calendar now = Calendar.getInstance();
		String year = Integer.toString(now.get(Calendar.YEAR));
		String month = Integer.toString(now.get(Calendar.MONTH)+1); if (month.length()==1) month = "0"+month;
		String day = Integer.toString(now.get(Calendar.DAY_OF_MONTH)); if (day.length()==1) day = "0"+day;
		String hour = Integer.toString(now.get(Calendar.HOUR_OF_DAY)); if (hour.length()==1) hour = "0"+hour;
		String minute = Integer.toString(now.get(Calendar.MINUTE)); if (minute.length()==1) minute = "0"+minute;
		String second = Integer.toString(now.get(Calendar.SECOND)); if (second.length()==1) second = "0"+second;
		String timestamp = "";
		if (includeDate) timestamp += year+month+day;
		if (includeHourAndMinute) timestamp += (includeDate ? "_" : "")+hour+minute;
		if (includeSeconds) timestamp += (includeDate || includeHourAndMinute ? "_" : "")+second;
		
        return timestamp;
	}
	
	public static String getTimestampedFilename(String prefix, String suffix) {
	    String fileName = prefix+createTimestamp(true, true, true)+"."+suffix;
	    
	    return fileName;
	}
	
	public static boolean pumpInStreamToOutStream(InputStream in, OutputStream out) {
		boolean success = false;
		try {
			// Read the resource into a local byte buffer 1k at a time.
			byte[] buffer = new byte[1024];
			int size = 0;
			while((size=in.read(buffer,0,1024))>=0){
				out.write(buffer,0,size);
			}
			
			success = true;
		} catch (IOException e) {
			System.err.println(TAG+"; IOException: "+e.getLocalizedMessage());
			e.printStackTrace();
			success = false;
		} finally {
		}
		return success;
	}
	
	public static boolean pumpInStreamToFile(InputStream in, File dst, boolean overwrite) {
		if (dst.exists()) {
			if (overwrite) {
				boolean deleted = dst.delete();
				if (!deleted) {
					System.err.println(TAG+"; Couldn't delete file in preparation for overwriting: "+dst.getAbsolutePath());
					return false;
				}
			} else {
				System.out.println(TAG+"; pumpResourceToFile returning without writing due to existing file and overwrite flag==false");
				return false;
			}
		}
		
		boolean success = false;
		OutputStream out = null;
		BufferedOutputStream bos = null;
		try {
			bos = new BufferedOutputStream(out = new FileOutputStream(dst));

			success = pumpInStreamToOutStream(in, bos);
		} catch (FileNotFoundException e) {
			System.err.println(TAG+"; Error: FileNotFound: "+dst.getAbsolutePath()+"; cannot write image."+ e);
			success = false;
		} finally {
			try {
				if (bos != null) bos.close();
				if (out != null) out.close();
			} catch (IOException ioe) {
				success = false;
			}
			bos = null;
			out = null;
		}
		return success;
	}
	
	public static boolean pumpFileToFile(File src, File dst, boolean overwrite) {
		boolean success = false;
		InputStream in = null;
		BufferedInputStream bis = null;
		try {
			bis = new BufferedInputStream(in = new FileInputStream(src));

			success = pumpInStreamToFile(in, dst, overwrite);
		} catch (FileNotFoundException e) {
			System.err.println(TAG+"; Error: FileNotFound: "+src.getAbsolutePath()+"; cannot read file."+ e);
			success = false;
		} finally {
			try {
				if (bis != null) bis.close();
				if (in != null) in.close();
			} catch (IOException ioe) {
				success = false;
			}
			bis = null;
			in = null;
		}
		
		return success;
	}
}
