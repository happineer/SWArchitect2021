#!/usr/bin/python

import os
import os.path
import argparse
from pyfiglet import figlet_format


class ArtifactManager:
    def __init__(self):
        self.provision_rootpath = "/home/lg/artifacts/version"
        self.server_rootpath = os.path.dirname(os.path.abspath(__file__))
        self.artifact_dict = {}
        self.artifact_list = ["engines", "svm", "faces", "svm", "temp.prototxt"]

    def get_list(self):
        return os.listdir(self.provision_rootpath)

    def show_artifact_list(self):
        artifact_list = self.get_list()

        print("[Artifact List]")
        for n, artifact in enumerate(artifact_list, 1):
            print("[{num}] artifact version({artifact_name})".format(num=n, artifact_name=artifact))
            self.artifact_dict[n] = artifact
        print("")

    def get_artifact_path(self, num):
        return os.path.join(self.provision_rootpath, self.artifact_dict[num])

    def clear(self):
        cmd_clear = "rm -rf %s" % (" ".join(self.artifact_list))
        print(cmd_clear)
        os.system(cmd_clear)

    def provision(self, identifier):
        if isinstance(identifier, int):
            provision_path = self.get_artifact_path(identifier)
        else:
            provision_path = os.path.join(self.provision_rootpath, identifier)

        if not os.path.isdir(provision_path):
            print("[Error] %s could not found." % provision_path)
            exit(-1)

        # add prefix to make an artifact to fullpath
        artifact_list = map(
                lambda artifact: os.path.join(provision_path, artifact),
                self.artifact_list)
        # provisioning artifacts to local path
        cmd = "cp -rf %s %s/" % (" ".join(artifact_list), self.server_rootpath)
        print(cmd)
        ret = os.system(cmd)



def show_summary(text):
    print("-" * 80); print(text); print("-" * 80)


def script_title():
    print(figlet_format('CMU-Team3', font='slant'))
#print(figlet_format('CMU-Team3', font='epic'))
#print(figlet_format('CMU-Team3', font='cosmike'))
#print(figlet_format('CMU-Team3', font='cybermedium'))
#print(figlet_format('CMU-Team3', font='doom'))

def mk_symlink():
    if not os.path.isfile("friends_1280x720_12fps.smjpeg"):
        symlink_cmd = "ln -s ~lg/artifacts/sample_video/friends_1280x720_12fps.smjpeg friends_1280x720_12fps.smjpeg"
        os.system(symlink_cmd)

def main():
    script_title()
    parser = argparse.ArgumentParser()
    parser.add_argument('--cmd', '-c', required=True, default=[], choices=['clear', 'provision', 'list'], help='a command list')
    parser.add_argument('--artifact-version', '-v', required=False, default=None, help='artifact version')
    opt = parser.parse_args()

    artifact_mgr = ArtifactManager()

    if opt.cmd == "clear":
        show_summary("* Command: clear artifacts at local path")
        artifact_mgr.clear()
    elif opt.cmd == "provision":
        show_summary("* Command: provisioning the artifacts to local path")
        if opt.artifact_version is not None:
            artifact_mgr.provision(opt.artifact_version)
        else:
            artifact_mgr.show_artifact_list()
            num = int(raw_input("Select the number of what you want to provision >> "))
            artifact_mgr.provision(num)
    elif opt.cmd == "list":
        show_summary("* Command: show the list of artifacts that support")
        artifact_mgr.show_artifact_list()

    mk_symlink()

if __name__ == "__main__":
    main()
