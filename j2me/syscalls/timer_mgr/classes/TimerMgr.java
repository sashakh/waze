import java.util.Timer;
import java.util.TimerTask;

public class TimerMgr {

	private class AppTimer extends TimerTask {
		int expire_count;
		int reset_count;

		public void run() {
			expire_count++;			
		}
	}

	private static final int MAX_TIMERS = 10;
	private AppTimer[] timers = new AppTimer[MAX_TIMERS];
	private static TimerMgr instance;
	private Timer m_timer = new Timer();

	private TimerMgr() {}

	public static TimerMgr getInstance()
	{
		if (instance == null) {
			instance = new TimerMgr();
		}

		return instance;
	}
	
	public int set(int interval) {
		int i;
		for (i=0; i<MAX_TIMERS; ++i) if (timers[i] == null) break;
		if (i == MAX_TIMERS) return -1;
		
		AppTimer app = new AppTimer();
		timers[i] = app;
		m_timer.schedule(app, interval, interval);
		
		return i;		
	}

	public void remove(int index) {
		timers[index].cancel();
		timers[index] = null;
	}
	
	public int getExpired() {
		for (int i=0; i<MAX_TIMERS; i++) {
			if ((timers[i] != null) &&
					(timers[i].expire_count != timers[i].reset_count)) {
				timers[i].reset_count = timers[i].expire_count;
				return i;
			}
		}
		return -1;
	}

}
