#!/bin/bash
#Written by chijure 


echo "  _____                      ____  _                      _        "
echo " |_   _|__  __ _ _ __ ___   |  _ \| |__   ___   ___ _ __ (_)_  __  "
echo "   | |/ _ \/ _` | '_ ` _ \  | |_) | '_ \ / _ \ / _ \ '_ \| \ \/ /  " 
echo "   | |  __/ (_| | | | | | | |  __/| | | | (_) |  __/ | | | |>  <   "
echo "   |_|\___|\__,_|_| |_| |_| |_|   |_| |_|\___/ \___|_| |_|_/_/\_\  "

cd kernelalto45lollipop
patch -p1 <patch/patch-3.10.28-29
patch -p1 <patch/patch-3.10.29-30
patch -p1 <patch/patch-3.10.30-31
patch -p1 <patch/patch-3.10.31-32
patch -p1 <patch/patch-3.10.32-33
patch -p1 <patch/patch-3.10.33-34
patch -p1 <patch/patch-3.10.34-35
patch -p1 <patch/patch-3.10.35-36
patch -p1 <patch/patch-3.10.36-37
patch -p1 <patch/patch-3.10.37-38
patch -p1 <patch/patch-3.10.38-39
patch -p1 <patch/patch-3.10.39-40
patch -p1 <patch/patch-3.10.40-41
patch -p1 <patch/patch-3.10.41-42
patch -p1 <patch/patch-3.10.42-43
patch -p1 <patch/patch-3.10.43-44
patch -p1 <patch/patch-3.10.44-45
patch -p1 <patch/patch-3.10.45-46
patch -p1 <patch/patch-3.10.46-47
patch -p1 <patch/patch-3.10.47-48
patch -p1 <patch/patch-3.10.48-49

echo "Patching kernel finished"
