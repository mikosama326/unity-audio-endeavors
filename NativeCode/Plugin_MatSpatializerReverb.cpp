// Simple Velvet-noise based reverb just to demonstrate how to send data from Spatializer to a common mix buffer that is routed into the mixer.

#include "AudioPluginUtil.h"

float matreverbmixbuffer[65536] = { 0 };

namespace MatSpacializerReverb
{
    const int MAXTAPS = 1024;

    enum
    {
        P_DELAYTIME,
        P_DIFFUSION,
        P_VOLUME,
        P_SA,
        P_NSIDES,
        P_NUM
    };

    struct InstanceChannel
    {
        struct Tap
        {
            int pos;
            float amp;
        };
        struct Delay
        {
            enum { MASK = 0xFFFFF };
            int writepos;
            inline void Write(float x)
            {
                writepos = (writepos + MASK) & MASK;
                data[writepos] = x;
            }

            inline float Read(int delay) const
            {
                return data[(writepos + delay) & MASK];
            }

            float data[MASK + 1];
        };
        Tap taps[1024];
        Delay delay;
    };

    struct EffectData
    {
        float p[P_NUM];
        Random random;
        InstanceChannel ch[2];
    };

    int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition)
    {
        int numparams = P_NUM;
        definition.paramdefs = new UnityAudioParameterDefinition[numparams];
        RegisterParameter(definition, "OLD Delay Time", "", 0.0f, 5.0f, 2.0f, 1.0f, 1.0f, P_DELAYTIME, "Delay time in seconds");
        RegisterParameter(definition, "OLD Diffusion", "%", 0.0f, 1.0f, 0.5f, 100.0f, 1.0f, P_DIFFUSION, "Diffusion amount");
        RegisterParameter(definition, "Volume", "m3", 0.01f, 1000.0f, 0.5f, 1.0f, 1.0f, P_VOLUME, "Volume");
        RegisterParameter(definition, "SA", "m2/s", 0.01f, 1000.0f, 0.5f, 1.0f, 1.0f, P_SA, "SA");
        RegisterParameter(definition, "Number of Walls", "", 1, 1024, 4, 1.0f, 1.0f, P_NSIDES, "Number of Walls");

        return numparams;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState* state)
    {
        EffectData* effectdata = new EffectData;
        memset(effectdata, 0, sizeof(EffectData));
        state->effectdata = effectdata;
        InitParametersFromDefinitions(InternalRegisterEffectDefinition, effectdata->p);
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState* state)
    {
        EffectData* data = state->GetEffectData<EffectData>();
        delete data;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState* state, int index, float value)
    {
        EffectData* data = state->GetEffectData<EffectData>();
        if (index >= P_NUM)
            return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        data->p[index] = value;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState* state, int index, float* value, char *valuestr)
    {
        EffectData* data = state->GetEffectData<EffectData>();
        if (index >= P_NUM)
            return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        if (value != NULL)
            *value = data->p[index];
        if (valuestr != NULL)
            valuestr[0] = 0;
        return UNITY_AUDIODSP_OK;
    }

    int UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState* state, const char* name, float* buffer, int numsamples)
    {
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(UnityAudioEffectState* state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int outchannels)
    {
        if (inchannels != 2 || outchannels != 2)
        {
            memcpy(outbuffer, inbuffer, length * outchannels * sizeof(float));
            return UNITY_AUDIODSP_OK;
        }

        EffectData* data = state->GetEffectData<EffectData>();

        
        float delaytime = data->p[P_DELAYTIME] * state->samplerate + 1.0f;
        int numtaps = (int)(data->p[P_DIFFUSION] * (MAXTAPS - 2) + 1);

        numtaps = (int)data->p[P_NSIDES];

        /*const float size = data->p[P_SIZE];
        float absorption_coeff = data->p[P_ABSORPTION] / 100.0f;
        const float distance = data->p[P_DISTANCE];*/

        float volume = data->p[P_VOLUME];
        float surfacearea = data->p[P_SA];

        float t60 = 0.1611f * volume / (surfacearea);// in seconds per meter

        //const float delay = delaytime;// t60;//distance / 343.0f; // assuming you're standing in the middle of this cube, the time it takes for the speed of sound to hit a wall and come back to you
        //delaytime = delay * state->samplerate + 1.0f;



        data->random.Seed(0);

        for (int c = 0; c < 2; c++)
        {
            InstanceChannel& ch = data->ch[c];
            const InstanceChannel::Tap* tap_end = ch.taps + numtaps;

            float decay = powf(0.01f, 1.0f / (float)numtaps);

            decay = t60;// *distance * 0.001;

            float p = 0.0f, amp = (decay - 1.0f) / (powf(decay, numtaps + 1) - 1.0f);
            InstanceChannel::Tap* tap = ch.taps;
            while (tap != tap_end)
            {
                p += data->random.GetFloat(0.0f, 100.0f);
                tap->pos = p;
                tap->amp = amp;
                amp *= decay;
                ++tap;
            }

            float scale = delaytime / p;
            tap = ch.taps;
            while (tap != tap_end)
            {
                tap->pos *= scale;
                ++tap;
            }

            for (int n = 0; n < length; n++)
            {
                ch.delay.Write(inbuffer[n * 2 + c] + matreverbmixbuffer[n * 2 + c]);

                float s = 0.0f;
                const InstanceChannel::Tap* tap = ch.taps;
                while (tap != tap_end)
                {
                    s += ch.delay.Read(tap->pos) * tap->amp;
                    ++tap;
                }

                outbuffer[n * 2 + c] = s;
            }
        }

        memset(matreverbmixbuffer, 0, sizeof(matreverbmixbuffer));

        return UNITY_AUDIODSP_OK;
    }
}
