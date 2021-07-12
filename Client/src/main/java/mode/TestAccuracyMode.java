package mode;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import javax.imageio.ImageIO;
import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.Frame;
import com.lge.cmuteam3.client.OnPlayListener;
import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class TestAccuracyMode extends BaseMode implements OnPlayListener {
	
	private int accuracyTestFrame = 1;
	private String currentPath = "";
	private String accuracyFolder = File.separator + "accuracy" + File.separator; 
	
	TestAccuracyMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Accuracy";
	}
	
	@Override
	public void start() {
		super.start();
		accuracyTestFrame = 1;
		Path currentRelativePath = Paths.get("");
		currentPath = currentRelativePath.toAbsolutePath().toString();		
		
		NetworkManager networkManager = NetworkManager.getInstance();
		appendUiLog("Test Accuracy Mode start");
		PlaybackManager.getInstance().getPlayer().setOnPlayListener(this);
		PlaybackManager.getInstance().play();
		networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_ACCURACY);
	}

	@Override
	public void stop() {
		super.stop();
		PlaybackManager.getInstance().getPlayer().setOnPlayListener(null);
	}

	@Override
	public void onDisplayImage(Frame frame) {
		BufferedImage image = frame.getFrameImage();
		File outputfile = new File(currentPath + accuracyFolder + (accuracyTestFrame++) + ".png");
	    try {
			ImageIO.write(image, "png", outputfile);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	public RunningButtonMode getRunningButtonMode() {
		return RunningButtonMode.DISABLE_ALL_EXCEPT_CURRENT;
	}
}
