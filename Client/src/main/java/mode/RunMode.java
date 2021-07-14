package mode;

import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class RunMode extends BaseMode {
	RunMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Run";
	}
	
	@Override
	public void start() {
		super.start();
		NetworkManager networkManager = NetworkManager.getInstance();
		PlaybackManager.getInstance().playDirect(Constants.CONTROL_VALUE_RUN);
		networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_RUN);
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
	
	@Override
    public boolean needRestore() {
        return true;
    }
}
