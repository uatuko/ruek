import { createResource, createSignal, For, Show } from 'solid-js';
import { A } from '@solidjs/router';

import { IoChevronBack, IoChevronForward, IoDocumentOutline } from 'solid-icons/io';

import Busy from './busy';

import './files.css';

function Files(props) {
	const [pageTokens, setPageTokens] = createSignal([]);
	const [token, setToken] = createSignal('');
	const [nextToken, setNextToken] = createSignal('');

	const prev = () => {
		const t = pageTokens().pop();
		setPageTokens(pageTokens());

		setToken(t);
	};

	const next = () => {
		setPageTokens([ ...pageTokens(), token() ]);

		const t = nextToken();
		setNextToken('');

		setToken(t);
	};

	const [files] = createResource(
		() => [token(), props.user.id],
		async ([token, userId]) => {
			const resp = await fetch(`http://localhost:3000/v1/files?pagination_limit=3&pagination_token=${token}`, {
				headers : {
					'user-id': userId,
				},
			});

			const data = await resp.json();
			setNextToken( data.pagination_token );

			return data.files;
		}
	);

	return (
		<div class="files">
			<div class="list">
				<div class="row header">
					<div class="cell">Name</div>
					<div class="cell">Access</div>
				</div>
				<Show
					when={!files.loading}
					fallback={Busy}
				>
					<For each={files()}>
						{
							(file) => (
								<div class="row">
									<div class="cell">
										<A class="link" href={`/files/${file.id}`}>
											<IoDocumentOutline />
											<span>{file.name}</span>
										</A>
									</div>
									<div class="cell">{file.role}</div>
								</div>
							)
						}
					</For>
				</Show>
			</div>
			<div class="pagination">
				<button
					disabled={files.loading || pageTokens().length === 0}
					onClick={prev}
				>
					<IoChevronBack />
				</button>
				<span>...</span>
				<button
					disabled={files.loading || nextToken() === ''}
					onClick={next}
				>
					<IoChevronForward />
				</button>
			</div>
		</div>
	);
}

export default Files;
