package mode;

import com.lge.cmuteam3.client.ui.UiController;

public class TestAccuracyMode extends BaseMode {
	TestAccuracyMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Accuracy";
	}
	
	@Override
	public void start() {
		appendUiLog("Test Accuracy Mode start");
		
	}

	@Override
	public void stop() {
		appendUiLog("Test Accuracy Mode stop");
		
	}

}
