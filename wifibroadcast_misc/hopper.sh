channellist=(2312M 2317M 2322M 2327M 2332M 2337M 2342M 2347M 2352M 2357M 2362M 2367M 2372M 2377M 2382M 2387M 2392M 2397M 2402M 2407M 2412M 2417M 2422M 2427M 2432M 2437M 2442M 2447M 2452M 2457M 2462M 2467M 2472M 2484M 2487M 2489M 2492M 2494M 2497M 2499M 2512M 2532M 2572M 2592M 2612M 2632M 2652M 2672M 2692M 2712)

    for channel in "${channellist[@]}"
        do
            iwconfig ec086b1c4645 channel $channel
            echo "$channel"
	    BLOCKS1=`/root/wifibroadcast/rx_alive_test`
	    sleep 0.05
	    BLOCKS2=`/root/wifibroadcast/rx_alive_test`
	    if [ $BLOCKS1 != $BLOCKS2 ]; then
		echo "channel found!"
		break
	    fi
        done
