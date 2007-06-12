import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.lcdui.game.*;
import java.lang.*;
import java.util.*;
import java.io.*;

public class CommandMgr implements CommandListener
{
  private Hashtable commands = new Hashtable();
  private int addr;
  private static CommandMgr instance;

  private CommandMgr() {}

  public static CommandMgr getInstance()
  {
    if (instance == null) {
      instance = new CommandMgr();
    }

    return instance;
  }

  public void setResultMem(int addr) {
    this.addr = addr;
  }

  public void addCommand(String name, int callback)
  {
    GameCanvas gc = (GameCanvas)CRunTime.getRegisteredObject(
                                        Syscalls.NOPH_GameCanvas_get());
    Command cmd = new Command(name, Command.SCREEN, 1);
    commands.put(cmd, new Integer(callback));
    gc.addCommand(cmd);
    if (commands.size() == 1) {
      gc.setCommandListener(this);
    }
  }

  public void commandAction(Command cmd, Displayable d) {
    Integer callback = (Integer)commands.get(cmd);
    System.out.println("In commandAction: callback:" + callback + " addr:" + addr);
    if (callback != null) {
      CRunTime.memoryWriteWord(addr, callback.intValue());
    }
  }
}

