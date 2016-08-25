pthread_mutex_t& PSoundChannelCoreAudio::GetReadMuteMutex(){
	static pthread_mutex_t isReadMute_Mutex = PTHREAD_MUTEX_INITIALIZER;
	return isReadMute_Mutex;
}


pthread_mutex_t& PSoundChannelCoreAudio::GetWriteMuteMutex(){
	static pthread_mutex_t isWriteMute_Mutex = PTHREAD_MUTEX_INITIALIZER;
	return isWriteMute_Mutex;
}

PBoolean& PSoundChannelCoreAudio::GetReadMute(){
	static PBoolean isReadMute(PFalse);
	return isReadMute;
}

PBoolean& PSoundChannelCoreAudio::GetWriteMute(){
	static PBoolean isWriteMute(PFalse);
	return isWriteMute;
}

pthread_mutex_t& PSoundChannelCoreAudio::GetIsMuteMutex(){
	if(direction == Recorder){
		return GetReadMuteMutex();
	} else {
		return GetWriteMuteMutex();
	}
}

PBoolean& PSoundChannelCoreAudio::isMute(){
	if(direction == Recorder){
		return GetReadMute();
	} else {
		return GetWriteMute();
	}
}




