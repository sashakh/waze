import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;

import javax.microedition.io.StreamConnection;
import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;


public class SoundMgr implements PlayerListener {

	private static final int MAX_LISTS = 2;
	private Integer[] sound_lists = new Integer[MAX_LISTS];
	private int current_list = -1;
	private int current_list_item = -1;

	private class SoundList {
		private static final int MAX_SOUND_LIST = 20;
		static final int SOUND_LIST_NO_FREE = 0x1;

		int flags;
		int count;
		String[] list = new String[MAX_SOUND_LIST];

		public SoundList(int flags) {
			this.flags = flags;
		}
	}

	private static SoundMgr instance;

	private SoundMgr() {}

	public static SoundMgr getInstance()
	{
		if (instance == null) {
			instance = new SoundMgr();
		}

		return instance;
	}

	private void playNextItem() {

		SoundList list =
			(SoundList)CRunTime.getRegisteredObject(sound_lists[current_list].intValue());

		if (current_list_item == list.count) {
			if ((list.flags & SoundList.SOUND_LIST_NO_FREE) == 0) {
				listFree (sound_lists[current_list].intValue());
			}
			sound_lists[current_list] = null;
			playNextList();
			return;
		}

		InputStream is = getClass().getResourceAsStream(list.list[current_list_item]);

		if (is == null) {
			System.out.println("Error creating InputStream "
					+ list.list[current_list_item]);
			playNextList();
			return;
		}

		try {
			Player p = Manager.createPlayer(is, "audio/mpeg");

			if (p == null) {
				System.out.println("Error creating Player "
						+ list.list[current_list_item]);
			} else {
				p.addPlayerListener(this);
				p.realize();        // Realize the player
				p.prefetch();       // Prefetch the player
				System.out.println("Realized Player: "
						+ list.list[current_list_item]);
				p.start();
				current_list_item++;
			}
		} catch (Exception e) {
			System.out.println(e);
			playNextList();
			return;
		}		
	}

	private void playNextList() {
		synchronized (sound_lists) {
			current_list = (current_list + 1) % MAX_LISTS;

			if (sound_lists[current_list] == null) {

				/* nothing to play */
				current_list = -1;
			}
		}

		if (current_list == -1) return;

		current_list_item = 0;

		playNextItem();
	}

	public void playerUpdate(Player p, String event, Object eventData) {
		if (event == END_OF_MEDIA) {
			p.close();
			playNextItem();
		}
	}

	public int listCreate(int flags) {
		SoundList list = new SoundList(flags);
		return CRunTime.registerObject(list);
	}

	public int listAdd(int _list, String name) {
		SoundList list = (SoundList)CRunTime.getRegisteredObject(_list);

		if (list.count == SoundList.MAX_SOUND_LIST) return -1;
		list.list[list.count] = name;
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
			if (current_list != -1) {
				/* playing */
				int next = (current_list + 1) % MAX_LISTS;

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
