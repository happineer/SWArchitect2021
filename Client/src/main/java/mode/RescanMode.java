package mode;

import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.ui.UiController;

public class RescanMode extends BaseMode {
	RescanMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Rescan";
	}
	
	@Override
	public void start() {
		appendUiLog("Rescan Mode start");
	}

	@Override
	public void stop() {
		appendUiLog("Rescan Mode stop");
		PlaybackManager.getInstance().stop();
	}

}
