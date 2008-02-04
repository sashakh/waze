import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;

import javax.microedition.io.StreamConnection;
import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.control.VolumeControl;

public class SoundMgr implements PlayerListener {

	private class SoundList {
		private static final int MAX_SOUND_LIST = 20;
		static final int SOUND_LIST_NO_FREE = 0x1;

		int flags;
		int count;
		String[] list = new String[MAX_SOUND_LIST];
		InputStream[] streams;

		public SoundList(int flags) {
			this.flags = flags;
		}
	}

	private static final int MAX_LISTS = 2;
	private SoundList current_list;
	private Integer[] sound_lists = new Integer[MAX_LISTS];
	private int current_list_id = -1;
	private int current_list_item = -1;

	private static SoundMgr instance;

	private SoundMgr() {}

	public static SoundMgr getInstance()
	{
		if (instance == null) {
			instance = new SoundMgr();
		}

		return instance;
	}

	private void closeCurrentList() {
		for (int i=0; i<current_list.streams.length; i++) {
			if (current_list.streams[i] != null) {
				try { current_list.streams[i].close(); } catch (Exception e) {}
				current_list.streams[i] = null;
			}
		}
	}

	private void playNextItem() {

		while ((current_list_item < current_list.count) &&
				(current_list.streams[current_list_item] == null)) {

			current_list_item++;
		}

		if (current_list_item == current_list.count) {
			closeCurrentList();
			if ((current_list.flags & SoundList.SOUND_LIST_NO_FREE) == 0) {
				listFree (sound_lists[current_list_id].intValue());
			}
			sound_lists[current_list_id] = null;
			current_list = null;
			playNextList();
			return;
		}

		try {
			Player p = Manager.createPlayer(current_list.streams[current_list_item], "audio/mpeg");

			if (p == null) {
				System.out.println("Error creating Player "
						+ current_list.list[current_list_item]);
				closeCurrentList();
				playNextList();
			} else {
				p.addPlayerListener(this);
				p.realize();        // Realize the player
				VolumeControl vc = (VolumeControl) p.getControl("VolumeControl");
				if(vc != null) {
					vc.setLevel(80);
				}
				p.prefetch();       // Prefetch the player
				current_list_item++;
				p.start();
			}
		} catch (Exception e) {
			System.out.println(e);
			closeCurrentList();
			playNextList();
			return;
		}		
	}

	private void playNextList() {
		synchronized (sound_lists) {
			current_list_id = (current_list_id + 1) % MAX_LISTS;

			if (sound_lists[current_list_id] == null) {

				/* nothing to play */
				current_list_id = -1;
			}
		}

		if (current_list_id == -1) return;

		current_list_item = 0;
		current_list = (SoundList)CRunTime.getRegisteredObject(sound_lists[current_list_id].intValue());
		if ((current_list.streams == null) || (current_list.streams.length != current_list.count)) {
			current_list.streams = new InputStream[current_list.count];
		}
		for (int i=0; i<current_list.count; i++) {
			try {
				current_list.streams[i] =
					getClass().getResourceAsStream(
						current_list.list[i]);
			} catch (Exception e) {
				System.out.println("Error creating sound stream:"
						+ current_list.list[i]);
			}
		}

		playNextItem();
	}

	public void playerUpdate(final Player p, final String event, Object eventData) {
		//System.out.println("playerUpdate: " + event);

		if ((event != END_OF_MEDIA) && (event != STOPPED)) return;

        new Thread()
        {
            public void run()
            {
				setPriority(Thread.MAX_PRIORITY);
				try {
					if(p.getState() == p.PREFETCHED) p.stop();
				} catch (Exception e) {}
				try {p.close();} catch (Exception e) {}

				if (event == STOPPED) current_list_item--;
				playNextItem();
            }
        }.start();
	}

	public int listCreate(int flags) {
		SoundList list = new SoundList(flags);
		return CRunTime.registerObject(list);
	}

	public int listAdd(int _list, String name) {
		SoundList list = (SoundList)CRunTime.getRegisteredObject(_list);

		if (list.count == SoundList.MAX_SOUND_LIST) return -1;
		list.list[list.count] = name + ".mp3";
		list.count++;

		return list.count - 1;
	}

	public int listCount(int _list) {
		SoundList list = (SoundList)CRunTime.getRegisteredObject(_list);
		return list.count;
	}

	void listFree(int _list) {
		CRunTime.deRegisterObject(_list);
	}

	int playList(int _list) {
		synchronized (sound_lists) {
			if (current_list_id != -1) {
				/* playing */
				int next = (current_list_id + 1) % MAX_LISTS;

				if (sound_lists[next] != null) {

					SoundList list =
						(SoundList)CRunTime.getRegisteredObject(sound_lists[next].intValue());

					if ((list.flags & SoundList.SOUND_LIST_NO_FREE) == 0) {
						listFree (sound_lists[next].intValue());
					}
				}

				sound_lists[next] = new Integer(_list);

				return 0;
			}
		}

		/* not playing */
		sound_lists[0] = new Integer(_list);
		playNextList();

		return 0;
	}
}
