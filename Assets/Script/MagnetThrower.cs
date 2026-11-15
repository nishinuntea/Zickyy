/*==============================================================================

    磁石を投げる		 [MagnetThrower.cs]
														 Author : Nishimura
														 Date   : 2026/11/14
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.InputSystem;

[RequireComponent(typeof(LineRenderer))]
public class MagnetThrower : MonoBehaviour 
{
    public GameObject magnetPrefab;   // プレイヤーのInspectorで設定

    [Header("Throw")]
    public float throwPower = 12f;
    public float upwardForce = 4.5f;
    public float spawnHeight = 0.8f;
    public float spawnForwardOffset = 1.5f;

    [Header("Preview")]
    public int lineSegments = 40;
    public float timeStep = 0.08f;
    public string fieldTag = "Field";

    private LineRenderer lineRenderer;
    private bool isAiming = false;
    private readonly RaycastHit[] rayBuffer = new RaycastHit[16];

    void Awake()
    {
        lineRenderer = GetComponent<LineRenderer>();
    }

    void Start()
    {
        lineRenderer.useWorldSpace = true;
        lineRenderer.startWidth = 0.08f;
        lineRenderer.endWidth = 0.08f;
        if (lineRenderer.sharedMaterial == null)
            lineRenderer.material = new Material(Shader.Find("Sprites/Default"));
        lineRenderer.startColor = Color.cyan;
        lineRenderer.endColor = Color.blue;
        lineRenderer.positionCount = 0;
        lineRenderer.enabled = false;
    }

    void Update()
    {
        var keyboard = Keyboard.current;
        if (keyboard == null) return;

        // Fキーを押している間：軌道を表示
        if (keyboard.fKey.isPressed)
        {
            isAiming = true;
            DrawTrajectory();
        }

        // Fキーを離した瞬間：投げる
        if (isAiming && keyboard.fKey.wasReleasedThisFrame)
        {
            isAiming = false;
            lineRenderer.positionCount = 0;
            lineRenderer.enabled = false;
            Throw();
        }
    }

    // プレイヤーの向き（水平）＋少し上向き
    Vector3 GetThrowDirection()
    {
        return (transform.forward + Vector3.up * 0.3f).normalized;
    }

    Vector3 GetStartVelocity()
    {
        return GetThrowDirection() * throwPower + Vector3.up * upwardForce;
    }

    Vector3 GetSpawnPosition()
    {
        return transform.position + transform.forward * spawnForwardOffset + Vector3.up * spawnHeight;
    }

    void DrawTrajectory()
    {
        lineRenderer.enabled = true;

        Vector3 startPos = GetSpawnPosition();
        Vector3 v = GetStartVelocity();

        Vector3[] points = new Vector3[lineSegments];
        int count = lineSegments;

        for (int i = 0; i < lineSegments; i++)
        {
            float t = i * timeStep;
            points[i] = startPos + v * t + 0.5f * Physics.gravity * t * t;

            if (i > 0 && TryFindFieldHit(points[i - 1], points[i], out Vector3 hit))
            {
                points[i] = hit;
                count = i + 1;
                break;
            }
        }

        lineRenderer.positionCount = count;
        lineRenderer.SetPositions(points);
    }

    // 自分か親のどれかにFieldタグが付いていればOK
    bool IsField(Collider col)
    {
        Transform t = col.transform;
        while (t != null)
        {
            if (t.CompareTag(fieldTag)) return true;
            t = t.parent;
        }
        return false;
    }

    bool TryFindFieldHit(Vector3 from, Vector3 to, out Vector3 hitPoint)
    {
        hitPoint = to;
        Vector3 delta = to - from;
        float dist = delta.magnitude;
        if (dist < 0.0001f) return false;

        int n = Physics.RaycastNonAlloc(from, delta / dist, rayBuffer, dist,
            ~0, QueryTriggerInteraction.Ignore);

        float nearest = float.MaxValue;
        bool found = false;
        for (int i = 0; i < n; i++)
        {
            if (!IsField(rayBuffer[i].collider)) continue;
            if (rayBuffer[i].distance < nearest)
            {
                nearest = rayBuffer[i].distance;
                hitPoint = rayBuffer[i].point;
                found = true;
            }
        }
        return found;
    }

    void Throw()
    {
        if (magnetPrefab == null)
        {
            Debug.LogError("Magnet Prefabがインスペクターに設定されていません");
            return;
        }

        Vector3 velocity = GetStartVelocity();
        GameObject magnet = Instantiate(magnetPrefab, GetSpawnPosition(),
            Quaternion.LookRotation(GetThrowDirection()));

        Rigidbody rb = magnet.GetComponent<Rigidbody>();
        if (rb != null)
        {
            rb.isKinematic = false;
            rb.useGravity = true;
            rb.linearVelocity = velocity;
        }
    }
}
