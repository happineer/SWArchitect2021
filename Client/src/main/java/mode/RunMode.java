package mode;

import com.lge.cmuteam3.client.ui.BaseFrame;

public class RunMode implements Mode {

	@Override
	public String getModeName() {
		return "Run Mode";
	}
	
	@Override
	public void start() {
		BaseFrame.getInstance().appendLog("Run Mode start");
		
	}

	@Override
	public void stop() {
		BaseFrame.getInstance().appendLog("Run Mode stop");
	}

}
