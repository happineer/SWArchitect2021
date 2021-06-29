package mode;

import com.lge.cmuteam3.client.ui.BaseFrame;

public class TestAccuracyMode implements Mode {

	@Override
	public String getModeName() {
		return "Accuracy";
	}
	
	@Override
	public void start() {
		BaseFrame.getInstance().appendLog("Test Accuracy Mode start");
		
	}

	@Override
	public void stop() {
		BaseFrame.getInstance().appendLog("Test Accuracy Mode stop");
		
	}

}
