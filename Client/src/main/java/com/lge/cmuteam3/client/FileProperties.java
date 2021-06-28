package com.lge.cmuteam3.client;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import java.util.Set;

/**
 * Get properties from the given file.
 * using Singleton pattern
 */
public class FileProperties {
    private static final Logger LOG = LoggerFactory.getLogger(FileProperties.class);

    private static final String propFileName = "client.properties";

    private static FileProperties instance;

    private final Properties properties = new Properties();

    public String getProperty(String key) {
        return properties.getProperty(key);
    }

    public static synchronized FileProperties getInstance() {
        if (instance == null) {
            instance = new FileProperties();
            instance.load();
        }

        return instance;
    }

    public void load() {
        try (InputStream inputStream = getClass().getClassLoader().getResourceAsStream(propFileName)) {
            if (inputStream == null) {
                throw new FileNotFoundException("Not found:" + propFileName);
            }
            properties.load(inputStream);
            print();
        } catch (IOException e) {
            LOG.warn(e.getMessage());
        }
    }

    public void print() {
        Set<Object> objects = properties.keySet();
        int size = objects.size();
        if (size == 0) {
            LOG.error("No properties loaded");
            return;
        }

        LOG.info("Loaded properties... size : " + size);
        for (Object key: objects) {
            String strKey = String.valueOf(key);
            LOG.debug(" > " + strKey + " : " + properties.getProperty(strKey));
        }
    }

    public static String get(String key) {
        return getInstance().getProperty(key);
    }
}
