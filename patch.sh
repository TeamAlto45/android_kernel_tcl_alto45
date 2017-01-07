#!/bin/bash
#Written by chijure 


echo "  _____                      ____  _                      _        "
echo " |_   _|__  __ _ _ __ ___   |  _ \| |__   ___   ___ _ __ (_)_  __  "
echo "   | |/ _ \/ _` | '_ ` _ \  | |_) | '_ \ / _ \ / _ \ '_ \| \ \/ /  " 
echo "   | |  __/ (_| | | | | | | |  __/| | | | (_) |  __/ | | | |>  <   "
echo "   |_|\___|\__,_|_| |_| |_| |_|   |_| |_|\___/ \___|_| |_|_/_/\_\  "

cd kernelalto45lollipop
patch -p1 <patch/patch-3.10.30-31
patch -p1 <patch/patch-3.10.31-32

echo "Patching kernel finished"
