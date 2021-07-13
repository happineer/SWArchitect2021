package mode;

import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class RetroactiveRunMode extends BaseMode {
	RetroactiveRunMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "RetroactiveRun";
	}
	
	@Override
	public void start() {
		super.start();
		NetworkManager networkManager = NetworkManager.getInstance();
		PlaybackManager.getInstance().playDirect();
		networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_RARUN);
	}

	@Override
	public void stop() {
		super.stop();
		PlaybackManager.getInstance().stop();
	}

	@Override
	public RunningButtonMode getRunningButtonMode() {
		return RunningButtonMode.DISABLE_ALL_EXCEPT_CURRENT;
	}
}
