import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.lcdui.game.*;
import java.lang.*;
import java.util.*;
import java.io.*;

public class FormCommandMgr implements CommandListener,ItemStateListener
{
  private class CallBackContext {
    public int addr;
    public String name;
    public int context;

    public CallBackContext(int addr, String name, int context) {
    	this.addr = addr;
	this.name = name;
	this.context = context;
    }
  }

  private int button_id = 0;
  private static CallBackContext callback;
  private static Integer lock = new Integer(0);

  public static int getCallBackNotif(int callback_addr, int callback_name,
                                     int callback_context) {

    if (callback == null) return 0;

    synchronized (lock) {
       CRunTime.memoryWriteWord(callback_addr, callback.addr);
       int len = callback.name.getBytes().length;
       CRunTime.memcpy(callback_name, callback.name.getBytes(), 0, len);
       CRunTime.memoryWriteByte(callback_name+len, 0);
       CRunTime.memoryWriteWord(callback_context, callback.context);
       callback = null;
    }

    return 1;
  }

  private Hashtable commands = new Hashtable();
  private Form form;

  public FormCommandMgr(Form form) {
    this.form = form;
    form.setCommandListener(this);
    form.setItemStateListener(this);
  }

  public void addCommand(String name, int c_addr, String c_name, int c_context)
  {
    Command cmd = new Command(name, Command.SCREEN, ++button_id);
    commands.put(cmd, new CallBackContext(c_addr, c_name, c_context));
    form.addCommand(cmd);
  }

  public void addCallback(Item item, int c_addr, String c_name, int c_context)
  {
    commands.put(item, new CallBackContext(c_addr, c_name, c_context));
  }

  private void checkAction(Object obj) {
    CallBackContext callback = (CallBackContext)commands.get(obj);
    if (callback != null) {
      synchronized (lock) {
         FormCommandMgr.callback = callback;
      }
    }
  }

  public void commandAction(Command cmd, Displayable d) {
    checkAction(cmd);
  }

  public void itemStateChanged(Item item)
  {
    checkAction(item);
  }
}

