from PyInstaller.__main__ import run

if __name__ == '__main__':
    opts = ["ORLS.py",
            "--onefile",
            "--windowed",
            "--name=ORLS",
            "--icon=ORLS.ico",
            "--add-data",
            "ORLS.ico;."
            ] 

    run(opts)
