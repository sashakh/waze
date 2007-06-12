import com.nokia.mid.ui.*;

public class DeviceSpecific implements Runnable {

	private DeviceControl deviceControl;
	private boolean threadRunning;
	private int cycles,cycleTimeout; 

	public static void init() {
	        new DeviceSpecific();
	}

	public DeviceSpecific() {	
		cycleTimeout = 20000;
		threadRunning = true;
		new Thread(this).start();
	}

	public void stop() {
		threadRunning = false;
	}

	public void run() {
		while(threadRunning) {

			cycles++;

			//if(cycles==2) {
			//	cycleTimeout = 60000;
			//}

			deviceControl.setLights(0,0);
			deviceControl.setLights(0,100);

			try {
				Thread.currentThread().sleep(cycleTimeout);
			} catch(Exception e) {}	    
		}
	}
}

