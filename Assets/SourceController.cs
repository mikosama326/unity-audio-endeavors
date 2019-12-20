using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Audio;
using UnityEngine.UI;

public class SourceController : MonoBehaviour
{
    public AudioSource source;

    public void Play()
    {
        source.Play();
    }

    public void Pause()
    {
        source.Pause();
    }
}
