"""Publish one reusable PR comment per failed CI check and configuration."""
import json
import os
import re
import urllib.request
from pathlib import Path


def github_api(method, path, payload=None):
    data = json.dumps(payload, ensure_ascii=False).encode() if payload is not None else None
    request = urllib.request.Request(
        os.environ.get('GITHUB_API_URL', 'https://api.github.com') + path,
        data=data, method=method,
        headers={'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'],
                 'Accept': 'application/vnd.github+json',
                 'X-GitHub-Api-Version': '2022-11-28',
                 'Content-Type': 'application/json', 'User-Agent': 'Windows-CI-report'})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def diagnostics(path):
    if not path.is_file():
        return '오류 로그가 생성되지 않았습니다. 아래 Actions 링크에서 실패한 단계를 확인하세요.'
    raw = path.read_bytes()
    text = raw.decode('utf-16' if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig', errors='replace')
    lines = text.splitlines()
    errors = [line.strip() for line in lines if re.search(
        r'\b(?:fatal\s+)?error\b|clang-diagnostic-error|Traceback', line, re.I)]
    selected = list(dict.fromkeys(errors))[:40] if errors else lines[-35:]
    return '\n'.join(selected)[:7000].replace('```', '` ` `')


def make_body(check, configuration, sha, run_url, log):
    marker = f'<!-- windows-ci-failure:{check}:x64:{configuration} -->'
    return marker, (
        f'{marker}\n### ❌ {check} — x64 {configuration}\n\n'
        f'검사가 실패했습니다. 검사한 커밋: `{sha[:12]}`\n\n'
        f'[Actions 실행 및 전체 로그]({run_url})\n\n'
        '<details>\n<summary>오류 요약</summary>\n\n'
        f'```text\n{diagnostics(log)}\n```\n\n</details>\n\n'
        '오류 요약이 없거나 빌드가 성공한 로그만 보이면 Actions에서 도구 설정·로그 업로드 등 후속 단계도 확인하세요.')


def main():
    # A successful job must not read logs, request a token, or publish a comment.
    if os.environ.get('REPORT_JOB_STATUS') != 'failure' or os.environ.get('GITHUB_EVENT_NAME') != 'pull_request':
        return
    event = json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text(encoding='utf-8'))
    repository = os.environ['GITHUB_REPOSITORY']
    if event['pull_request']['head']['repo']['full_name'] != repository:
        return
    check, configuration = os.environ['REPORT_CHECK'], os.environ['REPORT_CONFIGURATION']
    if check not in {'MSBuild', 'MSVC Analysis', 'clang-tidy'} or configuration not in {'Debug', 'Release'}:
        raise ValueError('Unsupported CI check or configuration')
    number = int(event['number'])
    run_url = f"{os.environ['GITHUB_SERVER_URL']}/{repository}/actions/runs/{os.environ['GITHUB_RUN_ID']}"
    marker, body = make_body(check, configuration, event['pull_request']['head']['sha'], run_url,
                             Path(os.environ['REPORT_LOG']))
    endpoint = f'/repos/{repository}/issues/{number}/comments'
    page = 1
    while True:
        comments = github_api('GET', f'{endpoint}?per_page=100&page={page}')
        for comment in comments:
            if (comment['user']['login'] == 'github-actions[bot]' and
                    comment['body'].startswith(marker)):
                github_api('PATCH', f"/repos/{repository}/issues/comments/{comment['id']}", {'body': body})
                return
        if len(comments) < 100:
            break
        page += 1
    github_api('POST', endpoint, {'body': body})


if __name__ == '__main__':
    main()
