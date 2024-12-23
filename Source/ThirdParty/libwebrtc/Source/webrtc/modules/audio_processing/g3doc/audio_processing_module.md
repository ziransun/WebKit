<!-- go/cmark -->
<!--* freshness: {owner: 'peah' reviewed: '2021-04-13'} *-->

# Audio Processing Module (APM)

## Overview

The APM is responsible for applying speech enhancements effects to the
microphone signal. These effects are required for VoIP calling and some
examples include echo cancellation (AEC), noise suppression (NS) and
automatic gain control (AGC).

The API for APM resides in [`/api/audio/audio_processing.h`][https://webrtc.googlesource.com/src/+/refs/heads/main/api/audio/audio_processing.h].
APM is created using the [`BuiltinAudioProcessingBuilder`][https://webrtc.googlesource.com/src/+/refs/heads/main/api/audio/builtin_audio_processing_builder.h]
builder that allows it to be customized and configured.

Some specific aspects of APM include that:
*  APM is fully thread-safe in that it can be accessed concurrently from
   different threads.
*  APM handles for any input sample rates < 384 kHz and achieves this by
   automatic reconfiguration whenever a new sample format is observed.
*  APM handles any number of microphone channels and loudspeaker channels, with
   the same automatic reconfiguration as for the sample rates.


APM can either be used as part of the WebRTC native pipeline, or standalone.
