/*==============================================================================

    投げる磁石		 [MagnetProjectile.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;

public class MagnetProjectile : MonoBehaviour
{
    public enum StickOrientation
    {
        UpAxisOut,       // モデルの上(Y+)が壁の外向き
        ForwardAxisOut,  // モデルの前(Z+)が壁の外向き
        BackAxisOut,     // モデルの後ろ(Z-)が壁の外向き
        RightAxisOut     // モデルの右(X+)が壁の外向き
    }

    [Header("Tags")]
    public string fieldTag = "Field";
    public string movableTag = "MovalObject";

    [Header("Stick Settings")]
    public float surfaceOffset = 0.0f;   // 壁・床から少し浮かせたい時に増やす
    public StickOrientation stickOrientation = StickOrientation.UpAxisOut;
    public bool snapToAxis = true;       // 面の向きをワールド軸に揃えて斜めを防ぐ
    public float maxFlightTime = 10f;    // どこにも張り付かない時に消えるまでの時間

    [Header("Attract Settings")]
    public float attractRadius = 8f;
    public float pullSpeed = 12f;
    public float stopDistance = 0.8f;
    public float attractDuration = 3f;
    public float lifeTimeAfterAttract = 0.5f;

    [Header("Glass Break Settings")]
    public string glassLayerName = "Glass";    // このレイヤーのオブジェクトだけを壊す
    public float glassCheckScale = 1.2f;       // 引き寄せ中の物の周囲をどれだけ広く判定するか

    private Rigidbody rb;
    private bool isStuck = false;
    private bool isAttracting = false;
    private float attractEndTime;
    private Vector3 lastPos;
    private Collider stuckCollider;

    // 動く壁への追従用
    private Transform followTarget;
    private Vector3 localPos;
    private Quaternion localRot;

    private readonly Collider[] hitBuffer = new Collider[64];
    private readonly Collider[] hitBuffer2 = new Collider[16];
    private readonly RaycastHit[] rayBuffer = new RaycastHit[16];

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        rb.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic;

        // すべてのコライダーをトリガーにして、物理的には何にも当たらないようにする
        foreach (var col in GetComponentsInChildren<Collider>())
        {
            col.isTrigger = true;
        }
    }

    void Start()
    {
        lastPos = rb.position;
        Invoke(nameof(DestroySelf), maxFlightTime); // 張り付いたらCancelInvokeで止まる
    }

    void DestroySelf()
    {
        Destroy(gameObject);
    }

    void FixedUpdate()
    {
        if (!isStuck)
        {
            CheckFieldHit();
            return;
        }

        if (isAttracting)
        {
            UpdateAttract();
        }
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

    // 飛行中：前回位置から今の位置までをレイキャストして、Fieldだけを探す
    void CheckFieldHit()
    {
        Vector3 currentPos = rb.position;
        Vector3 delta = currentPos - lastPos;
        float distance = delta.magnitude;

        if (distance > 0.0001f)
        {
            int count = Physics.RaycastNonAlloc(
                lastPos, delta / distance, rayBuffer, distance,
                ~0, QueryTriggerInteraction.Ignore);

            // 一番近いFieldだけを採用（それ以外はすり抜ける）
            float nearest = float.MaxValue;
            int nearestIndex = -1;
            for (int i = 0; i < count; i++)
            {
                if (!IsField(rayBuffer[i].collider)) continue;
                if (rayBuffer[i].distance < nearest)
                {
                    nearest = rayBuffer[i].distance;
                    nearestIndex = i;
                }
            }

            if (nearestIndex >= 0)
            {
                StickTo(rayBuffer[nearestIndex]);
                return;
            }
        }

        lastPos = currentPos;
    }

    void StickTo(RaycastHit hit)
    {
        isStuck = true;
        stuckCollider = hit.collider;
        CancelInvoke();

        rb.linearVelocity = Vector3.zero;
        rb.angularVelocity = Vector3.zero;
        rb.isKinematic = true;
        rb.useGravity = false;

        Vector3 normal = hit.normal;
        if (snapToAxis) normal = SnapToAxis(normal);

        transform.rotation = GetStickRotation(normal);
        transform.position = hit.point + normal * surfaceOffset;

        // 動く物に当たった場合だけ、相対位置を記録して追従する
        Rigidbody hitRb = hit.collider.attachedRigidbody;
        if (hitRb != null)
        {
            followTarget = hitRb.transform;
            localPos = followTarget.InverseTransformPoint(transform.position);
            localRot = Quaternion.Inverse(followTarget.rotation) * transform.rotation;
        }
    }

    // 法線を一番近いワールド軸(上下左右前後)に丸める
    Vector3 SnapToAxis(Vector3 n)
    {
        float ax = Mathf.Abs(n.x);
        float ay = Mathf.Abs(n.y);
        float az = Mathf.Abs(n.z);

        if (ax >= ay && ax >= az) return new Vector3(Mathf.Sign(n.x), 0f, 0f);
        if (ay >= ax && ay >= az) return new Vector3(0f, Mathf.Sign(n.y), 0f);
        return new Vector3(0f, 0f, Mathf.Sign(n.z));
    }

    Quaternion GetStickRotation(Vector3 normal)
    {
        // 床・天井はforwardを基準、壁はワールドの上方向を基準にして横に傾かないようにする
        Vector3 refUp = Mathf.Abs(normal.y) > 0.99f ? Vector3.forward : Vector3.up;
        Quaternion faceNormal = Quaternion.LookRotation(normal, refUp); // Z+が法線を向く

        // モデルの「壁に向けたい軸」をZ+に合わせる回転
        Vector3 modelAxis;
        switch (stickOrientation)
        {
            case StickOrientation.ForwardAxisOut: modelAxis = Vector3.forward; break;
            case StickOrientation.BackAxisOut: modelAxis = Vector3.back; break;
            case StickOrientation.RightAxisOut: modelAxis = Vector3.right; break;
            case StickOrientation.UpAxisOut:
            default: modelAxis = Vector3.up; break;
        }

        return faceNormal * Quaternion.FromToRotation(modelAxis, Vector3.forward);
    }

    // 張り付いた後にMovalObjectが触れたら吸着開始
    void OnTriggerEnter(Collider other)
    {
        if (!isStuck || isAttracting) return;
        if (other.CompareTag(movableTag))
        {
            isAttracting = true;
            attractEndTime = Time.time + attractDuration;
        }
    }

    void LateUpdate()
    {
        if (!isStuck || followTarget == null) return;

        transform.position = followTarget.TransformPoint(localPos);
        transform.rotation = followTarget.rotation * localRot;
    }

    // 引き寄せ中のMovalObjectの周りにあるGlassを壊す（Glassレイヤー以外は壊さない）
    void BreakGlassAround(Collider movable)
    {
        int glassLayer = LayerMask.NameToLayer(glassLayerName);
        if (glassLayer == -1) return;

        Bounds b = movable.bounds;
        int count = Physics.OverlapBoxNonAlloc(
            b.center, b.extents * glassCheckScale, hitBuffer2, Quaternion.identity,
            1 << glassLayer, QueryTriggerInteraction.Ignore);

        for (int i = 0; i < count; i++)
        {
            Collider c = hitBuffer2[i];
            if (c == null) continue;

            // 磁石が張り付いているガラスが壊れる場合は、磁石も一緒に消す
            if (c == stuckCollider)
            {
                Destroy(c.gameObject);
                Destroy(gameObject);
                return;
            }

            Destroy(c.gameObject);
        }
    }

    void UpdateAttract()
    {
        if (Time.time >= attractEndTime)
        {
            isAttracting = false;
            Destroy(gameObject, lifeTimeAfterAttract);
            return;
        }

        int count = Physics.OverlapSphereNonAlloc(
            transform.position, attractRadius, hitBuffer,
            ~0, QueryTriggerInteraction.Ignore);

        for (int i = 0; i < count; i++)
        {
            Collider col = hitBuffer[i];
            if (!col.CompareTag(movableTag)) continue;

            Rigidbody targetRb = col.attachedRigidbody;
            if (targetRb == null || targetRb.isKinematic) continue;

            Vector3 toMagnet = transform.position - targetRb.position;
            float dist = toMagnet.magnitude;

            if (dist <= stopDistance)
            {
                targetRb.linearVelocity = Vector3.zero;
            }
            else
            {
                targetRb.WakeUp();
                targetRb.linearVelocity = toMagnet.normalized * pullSpeed;
                BreakGlassAround(col);   // 動いている最中にガラスに触れたら壊す
            }
        }
    }

    void OnDrawGizmosSelected()
    {
        Gizmos.color = Color.cyan;
        Gizmos.DrawWireSphere(transform.position, attractRadius);
    }
}
