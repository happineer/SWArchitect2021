package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.network.OnConnectListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.lge.cmuteam3.client.ui.BaseFrame;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.*;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.awt.image.BufferedImage;
import java.util.Queue;
import javax.imageio.ImageIO;

public class Receiver extends Thread {
	private static final Logger LOG = LoggerFactory.getLogger(Receiver.class);
	private final Socket tcpSocket;

	private int bufferSize;

	private Queue<BufferedImage> queue = new LinkedList<>();

	private boolean imageReady = false;

	private transient boolean running = false;

	private OnConnectListener onConnectlistener;

	public Receiver(Socket socket) {
		LOG.info("socket:" + socket);
		this.tcpSocket = socket;
		String strBufferSize = FileProperties.get("client.bufferSize");
		this.bufferSize = Integer.parseInt(strBufferSize);
	}

	private void receiveImages() {
		try {
			if (tcpSocket == null) {
				LOG.info("Receiver:Socket is null!");
				return;
			}
			LOG.info("Receiver:Socket connected!");
			InputStream inputStream = tcpSocket.getInputStream();
			long init = System.currentTimeMillis();

			onConnectlistener.onConnected();

			running = true;

			/* Receiving loop */
			int count = 0;
			while (running) {
				byte[] sizeAr = new byte[4];
				readNBytes(inputStream, sizeAr, 0, 4);
				int size = ByteBuffer.wrap(sizeAr).asIntBuffer().get();
				
				// TODO: refactoring
				doMonitor(size);

				if (size == 0) {
					continue;
				}

				byte[] imageAr = new byte[size];
				readNBytes(inputStream, imageAr, 0, size);
				queue.add(ImageIO.read(new ByteArrayInputStream(imageAr)));
				long current = System.currentTimeMillis();
				init = current;

				count++;
				if (count >= bufferSize) {
					imageReady = true;
				}

				Thread.sleep(20);
			}
			running = false;
			LOG.info("receiving stopped.");
		} catch (ConnectException ce) {
			LOG.error(ce.getMessage());
			onConnectlistener.onFailed();
		} catch (IOException e) {
			e.printStackTrace();
			onConnectlistener.onFailed();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} finally {
			try {
				if (tcpSocket != null) {
					tcpSocket.close();
				}
				running = false;
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	@Override
	public void run() {
		LOG.info("Receiver run");
		receiveImages();
	}

	public BufferedImage getImageFrame(){
		if (imageReady) {
			return queue.poll();
		}
		return null;
	}

	public int getRemainBufferSize() {
		return queue.size();
	}
	
	// receive과정에 문제 여부를 확인한다.
	public void doMonitor(int size) {

		LOG.info("received frame size : " + size);
	}

	// Utils Refactoring
	private int readNBytes(InputStream inputStream, byte[] b, int off, int len) throws IOException {
        int n = 0;
        while (n < len) {
            int count = inputStream.read(b, off + n, len - n);
            if (count < 0)
                break;
            n += count;
        }
        return n;
    }

	public void stopSelf() {
		this.running = false;
	}

	public void setOnConnectListener(OnConnectListener onConnectlistener) {
		this.onConnectlistener = onConnectlistener;
	}
}