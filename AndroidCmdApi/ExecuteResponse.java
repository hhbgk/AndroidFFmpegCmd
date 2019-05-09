package tv.danmaku.ijk.media.player.ffmpeg.cmd;

public abstract class ExecuteResponse implements IFFmpegExecuteResponse {
    /**
     * on Progress
     * @param message current output of FFmpeg command
     */
    public abstract void onProgress(long message);
}
