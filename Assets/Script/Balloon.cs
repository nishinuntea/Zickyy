/*==============================================================================

    バルーン		 [Balloon.cs]
														 Author : Nishimura
														 Date   : 2026/09/28
--------------------------------------------------------------------------------

==============================================================================*/
using System.Collections;
using UnityEngine;

public class Balloon : MonoBehaviour
{
    [Header("ステータス")]
    [SerializeField] private int maxHealth = 1;            // 風船が割れるまでの回数（整数）
    private int currentHealth;                              // 現在のHP（整数）

    [Header("衝突検知の設定")]
    [SerializeField] private string targetTag = "MovalObject"; // 当てるオブジェクトのタグ
    [SerializeField] private float minImpactVelocity = 3.0f;   // 割れるために必要な最低限の衝突速度
    [SerializeField] private int damagePerHit = 1;             // 1回のヒットで与えるダメージ（整数）

    [Header("シェイク（揺れ）設定")]
    [SerializeField] private float shakeDuration = 0.15f; // 揺れる時間（秒）
    [SerializeField] private float shakeAmount = 0.1f;    // 揺れの強さ（距離）

    [Header("リスポーン設定")]
    [SerializeField] private float respawnTime = 3f;      // 復活までの待ち時間

    private Vector3 originalPosition;
    private Vector3 originalScale;
    private Collider balloonCollider;
    private Renderer balloonRenderer;
    private Coroutine shakeCoroutine;
    private bool isDead = false;

    private void Start()
    {
        originalPosition = transform.localPosition;
        originalScale = transform.localScale;
        balloonCollider = GetComponent<Collider>();
        balloonRenderer = GetComponent<Renderer>();

        ResetBalloon();
    }

    // --- 物理的な衝突（ぶつかった瞬間）を検知 ---
    private void OnCollisionEnter(Collision collision)
    {
        if (isDead) return;

        // 1. 相手が指定したタグ（MovalObject）かチェック
        if (collision.gameObject.CompareTag(targetTag))
        {
            // 2. 衝突時の相対速度（どれくらいの勢いでぶつかったか）を計算
            float hitVelocity = collision.relativeVelocity.magnitude;

            Debug.Log($"何かが衝突! 速度: {hitVelocity:F1} (必要条件: {minImpactVelocity})");

            // 3. 一定以上の勢いがある場合のみダメージを与える
            if (hitVelocity >= minImpactVelocity)
            {
                TakeDamage(damagePerHit);
            }
        }
    }

    // ダメージ処理
    public void TakeDamage(int damage)
    {
        if (isDead) return;

        currentHealth -= damage;
        Debug.Log($"Balloon Hit! 残りHP: {currentHealth}");

        // 連続でヒットした場合は前のシェイクをリセットして新しく揺らす
        if (shakeCoroutine != null)
        {
            StopCoroutine(shakeCoroutine);
        }
        shakeCoroutine = StartCoroutine(ShakeAnimation());

        // HPが0になったら破裂（非表示）
        if (currentHealth <= 0)
        {
            Die();
        }
    }

    // モデル全体の振動（シェイク）処理
    private IEnumerator ShakeAnimation()
    {
        float elapsedTime = 0f;

        while (elapsedTime < shakeDuration)
        {
            Vector3 randomOffset = Random.insideUnitSphere * shakeAmount;
            transform.localPosition = originalPosition + randomOffset;

            elapsedTime += Time.deltaTime;
            yield return null;
        }

        transform.localPosition = originalPosition;
        shakeCoroutine = null;
    }

    // 破壊処理
    private void Die()
    {
        isDead = true;

        if (shakeCoroutine != null)
        {
            StopCoroutine(shakeCoroutine);
            shakeCoroutine = null;
        }

        // コリジョンと描画をオフにして「消えた」状態にする
        balloonCollider.enabled = false;
        balloonRenderer.enabled = false;

        // リスポーン（復活）タイマー開始
        StartCoroutine(RespawnRoutine());
    }

    // 復活処理
    private IEnumerator RespawnRoutine()
    {
        yield return new WaitForSeconds(respawnTime);

        ResetBalloon();

        // ポップアップアニメーション（スケール0から1へ「ポンッ」と拡大して復活）
        transform.localScale = Vector3.zero;
        balloonCollider.enabled = true;
        balloonRenderer.enabled = true;

        float timer = 0f;
        float popSpeed = 5f; // 復活の速さ

        while (timer < 1f)
        {
            timer += Time.deltaTime * popSpeed;
            transform.localScale = Vector3.Lerp(Vector3.zero, originalScale, timer);
            yield return null;
        }

        transform.localScale = originalScale;
    }

    // 初期化・リセット
    private void ResetBalloon()
    {
        currentHealth = maxHealth;
        isDead = false;
        transform.localPosition = originalPosition;
        transform.localScale = originalScale;
    }
}