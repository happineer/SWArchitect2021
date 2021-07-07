package mode;

import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.ui.UiController;

public class LearnMode extends BaseMode {
	LearnMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Learn";
	}
	
	@Override
	public void start() {
		appendUiLog("Learn Mode start");
	}

	@Override
	public void stop() {
		appendUiLog("Learn Mode stop");
		PlaybackManager.getInstance().stop();
	}

}
