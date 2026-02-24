from PyInstaller.__main__ import run

if __name__ == '__main__':
    opts = ["ABEL.py",
            "--onefile",
            "--windowed",
            "--name=ABEL",
            "--icon=ABEL.ico",
            "--add-data",
            "ABEL.ico;."
            ] 
    run(opts)