import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import report_ci_failure as reporter


class FailureReportingTests(unittest.TestCase):
    def test_success_does_not_access_token_or_api(self):
        with patch.dict(os.environ, {'REPORT_JOB_STATUS': 'success'}, clear=True), patch.object(reporter, 'github_api') as api:
            reporter.main()
            api.assert_not_called()

    def test_summary_is_bounded_and_preserves_compiler_error(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / 'build.log'
            log.write_text('noise\n' * 3000 + 'LINK : fatal error LNK1181: GameEngine.lib\n', encoding='utf-16')
            _, body = reporter.make_body('MSBuild', 'Release', 'a' * 40, 'https://example.com/run', log)
            self.assertIn('LNK1181', body)
            self.assertNotIn('noise', body)
            self.assertLess(len(body), 9000)

    def test_failure_creates_or_updates_only_bot_comment(self):
        with tempfile.TemporaryDirectory() as directory:
            event_path = Path(directory) / 'event.json'
            event = {'number': 15, 'pull_request': {'head': {'repo': {'full_name': 'owner/repo'}, 'sha': 'a' * 40}}}
            event_path.write_text(json.dumps(event))
            env = {'REPORT_JOB_STATUS':'failure', 'GITHUB_EVENT_NAME':'pull_request', 'GITHUB_EVENT_PATH':str(event_path),
                   'GITHUB_REPOSITORY':'owner/repo', 'REPORT_CHECK':'clang-tidy', 'REPORT_CONFIGURATION':'Debug',
                   'REPORT_LOG':str(Path(directory)/'missing.log'), 'GITHUB_SERVER_URL':'https://github.com', 'GITHUB_RUN_ID':'123'}
            marker = '<!-- windows-ci-failure:clang-tidy:x64:Debug -->'
            for user, expected in [('human', 'POST'), ('github-actions[bot]', 'PATCH')]:
                with self.subTest(user=user), patch.dict(os.environ, env, clear=True), patch.object(reporter, 'github_api') as api:
                    api.return_value = [{'id':99, 'user':{'login':user}, 'body':marker}]
                    reporter.main()
                    self.assertEqual(api.call_args.args[0], expected)
            event['pull_request']['head']['repo']['full_name'] = 'fork/repo'
            event_path.write_text(json.dumps(event))
            with patch.dict(os.environ, env, clear=True), patch.object(reporter, 'github_api') as api:
                reporter.main()
                api.assert_not_called()


if __name__ == '__main__':
    unittest.main()
