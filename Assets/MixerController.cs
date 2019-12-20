using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Audio;
using UnityEngine.UI;

public enum Materials
{
    MARBLE,
    ASPHALT_tile,
    HEAVY_CARPET,
    PLATE_GLASS,
    TESTS
}

[System.Serializable]
public struct CoeffInfo
{
    public Materials Material;
    public Material material;
    public float AbsorptionCoefficient;
}

public class MixerController : MonoBehaviour
{
    public GameObject Room;
    public AudioMixer Mixer;
    public AudioMixerGroup MixerGroup;
    public List<CoeffInfo> Setup;

    [Header("Change these!")]
    public Materials Material;
    public float Size;

    private List<MeshRenderer> Walls = new List<MeshRenderer>();
    private Materials prevMat;

    void Start()
    {
        for (int i = 0; i < Room.transform.childCount; i++)
            Walls.Add(Room.transform.GetChild(i).GetComponent<MeshRenderer>());

        SetMaterial();
        SetParams();
    }

    // Update is called once per frame
    void Update()
    {
        if(prevMat != Material)
        {
            SetMaterial();
            prevMat = Material;
        }
        SetParams();
        Room.transform.localScale = new Vector3(1,1,1) * Size;
    }

    void SetMaterial()
    {
        for (int i = 0; i < Walls.Count; i++)
        {
            Walls[i].material = Setup[(int)Material].material;
        }
        
    }

    public void SetMaterial(int material)
    {
        this.Material = (Materials) material;
    }

    public void SetSize(GameObject size)
    {
        Size = size.GetComponent<Slider>().value;
    }

    void SetParams()
    {
        float SA = 4 * Size * Size * Setup[(int)Material].AbsorptionCoefficient;
        SetSA(1/SA);
        SetVolume(Mathf.Pow(Size, 3));
        SetDelay(Mathf.Lerp(0, 3, Size / 100f));
    }

    void SetVolume(float volume)
    {
        Mixer.SetFloat("RoomVolume", volume);
    }

    void SetSA(float SA)
    {
        Mixer.SetFloat("RoomSA", SA);
    }

    void SetDelay(float PreDelay)
    {
        Mixer.SetFloat("PreDelay", PreDelay);
    }
}
