import java.lang.*;
import java.util.*;
import java.io.*;
import javax.microedition.midlet.*;

public interface GpsIntr
{
  public int getURL(int addr, int size);
  public void searchGps(MIDlet m, String wait_msg, String not_found_msg);
  public int connect(String url);
  public void disconnect();
  public void start();
  public void stop();
  public int read(int addr, int size);
}

